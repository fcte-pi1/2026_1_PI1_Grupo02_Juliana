import { useEffect, useMemo, useRef, useState, type ReactNode } from 'react';
import { useSearchParams } from 'react-router-dom';
import { useQueryClient } from '@tanstack/react-query';
import { useLogout } from '@/domains/auth';
import {
  enviarComando,
  iniciarCorrida,
  useRunSnapshot,
  useTelemetryStream,
  useTentativas,
  type MazeState,
  type Pose,
  type RunSnapshot,
  type Tentativa,
} from '@/domains/runs';
import { TENTATIVAS_QUERY_KEY } from '@/domains/runs/hooks/useTentativas';
import './DashboardPage.css';

type TelemetryView = 'dashboard' | 'maze' | 'sensors' | 'runs' | 'run';
type Heading = 'Norte' | 'Leste' | 'Sul' | 'Oeste';
type RunStatus = 'Em curso' | 'Finalizada' | 'Abortada';

// Mapeia o heading do backend (N/S/E/W) para o rótulo em PT exibido na UI.
const HEADING_PT: Record<string, Heading> = { N: 'Norte', S: 'Sul', E: 'Leste', W: 'Oeste' };
const STATUS_PT: Record<string, string> = {
  em_curso: 'Run em andamento',
  finalizada: 'Finalizada',
  abortada: 'Abortada',
};
const STATUS_RUN_PT: Record<string, RunStatus> = {
  em_curso: 'Em curso',
  finalizada: 'Finalizada',
  abortada: 'Abortada',
};

interface SensorReading {
  label: 'Esquerda' | 'Frente' | 'Direita';
  value: number;
  status: 'Livre' | 'Parede';
}

interface RunRow {
  id: string;
  uuid: string;
  mouse: string;
  algorithm: string;
  start: string;
  duration: string;
  averageSpeed: number;
  cells: number;
  status: RunStatus;
}

interface TelemetrySnapshot {
  speed: number;
  peakSpeed: number;
  battery: number;
  voltage: number;
  elapsedSeconds: number;
  exploredCells: number;
  position: [number, number];
  heading: Heading;
  walls: number;
  sensors: SensorReading[];
  imu: {
    roll: number;
    pitch: number;
    yaw: number;
    bias: number;
  };
  history: number[];
  frontHistory: number[];
  events: Array<{
    time: string;
    type: 'EVT' | 'CMD' | 'WRN';
    name: string;
    detail: string;
  }>;
  maze: MazeState;
  dimensao: number;
  mazePose: Pose | null;
}

const EMPTY_SENSORS: SensorReading[] = [
  { label: 'Esquerda', value: 0, status: 'Livre' },
  { label: 'Frente', value: 0, status: 'Livre' },
  { label: 'Direita', value: 0, status: 'Livre' },
];

function tentativaToRow(t: Tentativa): RunRow {
  const durationSec =
    t.tempo_inicio && t.tempo_fim
      ? Math.floor(
          (new Date(t.tempo_fim).getTime() - new Date(t.tempo_inicio).getTime()) / 1000,
        )
      : null;
  return {
    id: `#${t.id.slice(0, 6).toUpperCase()}`,
    uuid: t.id,
    mouse: t.micromouse_nome || 'Mouse',
    algorithm: t.algoritmo || 'Flood Fill',
    start: t.tempo_inicio
      ? new Date(t.tempo_inicio).toLocaleString('pt-BR', {
          day: '2-digit',
          month: '2-digit',
          hour: '2-digit',
          minute: '2-digit',
        })
      : '-',
    duration: durationSec !== null ? formatDuration(durationSec) : t.tempo_inicio ? '...' : '-',
    averageSpeed: t.velocidade_media ?? 0,
    cells: t.explored,
    status: STATUS_RUN_PT[t.status] ?? 'Em curso',
  };
}

function formatDuration(seconds: number) {
  const hours = Math.floor(seconds / 3600);
  const minutes = Math.floor((seconds % 3600) / 60);
  const remainingSeconds = seconds % 60;
  return [hours, minutes, remainingSeconds].map((unit) => String(unit).padStart(2, '0')).join(':');
}

function buildSparkline(values: number[], width = 320, height = 84) {
  const min = Math.min(...values);
  const max = Math.max(...values);
  const range = max - min || 1;
  return values
    .map((value, index) => {
      const x = (index / (values.length - 1)) * width;
      const y = height - ((value - min) / range) * (height - 18) - 9;
      return `${x.toFixed(1)},${y.toFixed(1)}`;
    })
    .join(' ');
}


export function DashboardPage() {
  const [activeView, setActiveView] = useState<TelemetryView>('dashboard');
  const [now, setNow] = useState(0);
  const [lastPacketAt, setLastPacketAt] = useState(0);
  const [speedHistory, setSpeedHistory] = useState<number[]>(Array(12).fill(0));
  const [peakSpeed, setPeakSpeed] = useState(0);
  const [initiating, setInitiating] = useState(false);
  const logout = useLogout();
  const queryClient = useQueryClient();

  const [params, setParams] = useSearchParams();
  const tentativasQuery = useTentativas();
  const runId = params.get('run') ?? tentativasQuery.data?.[0]?.id ?? null;
  const snapshotQuery = useRunSnapshot(runId);
  const { snapshot: live, connected } = useTelemetryStream(runId);
  const real: RunSnapshot | null = live ?? snapshotQuery.data ?? null;

  // Relógio para atualizar o indicador "atualizado há Xs" e o cronômetro.
  useEffect(() => {
    const clock = window.setInterval(() => setNow(Date.now()), 250);
    return () => window.clearInterval(clock);
  }, []);

  // Atualiza o histórico de velocidade e o pico sempre que chega um novo pacote SSE.
  const prevLiveRef = useRef<RunSnapshot | null>(null);
  useEffect(() => {
    if (!live || live === prevLiveRef.current) return;
    prevLiveRef.current = live;
    setLastPacketAt(Date.now());
    if (live.speed != null) {
      setSpeedHistory((h) => [...h.slice(-11), live.speed!]);
      setPeakSpeed((p) => Math.max(p, live.speed!));
    }
  }, [live]);

  // Reseta histórico e pico quando muda de tentativa.
  useEffect(() => {
    setSpeedHistory(Array(12).fill(0));
    setPeakSpeed(0);
    prevLiveRef.current = null;
  }, [runId]);

  const elapsedSeconds = useMemo(() => {
    if (!real?.tempo_inicio) return 0;
    if (real.status === 'finalizada' && real.tempo_fim) {
      return Math.max(
        0,
        Math.floor(
          (new Date(real.tempo_fim).getTime() - new Date(real.tempo_inicio).getTime()) / 1000,
        ),
      );
    }
    return Math.max(0, Math.floor((now - new Date(real.tempo_inicio).getTime()) / 1000));
  }, [real, now]);

  const telemetry: TelemetrySnapshot = useMemo(() => {
    const pose = real?.pose ?? null;
    const walls = Object.values(real?.maze ?? {}).reduce(
      (acc, w) => acc + Number(w.n) + Number(w.s) + Number(w.e) + Number(w.w),
      0,
    );
    return {
      speed: real?.speed ?? 0,
      peakSpeed,
      battery: real?.battery ?? 0,
      voltage: real?.voltage ?? 0,
      elapsedSeconds,
      exploredCells: real?.explored ?? 0,
      position: pose ? ([pose.x, pose.y] as [number, number]) : ([0, 0] as [number, number]),
      heading: pose ? (HEADING_PT[pose.heading] ?? 'Norte') : 'Norte',
      walls,
      sensors: EMPTY_SENSORS,
      imu: { roll: 0, pitch: 0, yaw: 0, bias: 0 },
      history: speedHistory,
      frontHistory: Array(12).fill(0),
      events: [],
      maze: real?.maze ?? {},
      dimensao: real?.dimensao ?? 16,
      mazePose: pose,
    };
  }, [real, peakSpeed, elapsedSeconds, speedHistory]);

  const isRunning = real?.status === 'em_curso';
  const disconnected = !connected;
  const statusLabel = real ? (STATUS_PT[real.status] ?? real.status) : 'Aguardando sinal';
  const total = telemetry.dimensao * telemetry.dimensao;
  const exploredPercent = ((telemetry.exploredCells / total) * 100).toFixed(1);

  const recentRuns = useMemo(
    () => (tentativasQuery.data ?? []).slice(0, 4).map(tentativaToRow),
    [tentativasQuery.data],
  );
  const allRuns = useMemo(
    () => (tentativasQuery.data ?? []).map(tentativaToRow),
    [tentativasQuery.data],
  );
  const totalRuns = tentativasQuery.data?.length ?? 0;
  const runsEmCurso = tentativasQuery.data?.filter((t) => t.status === 'em_curso').length ?? 0;

  const handleIniciarRun = async () => {
    if (initiating) return;
    setInitiating(true);
    try {
      const tentativa = await iniciarCorrida();
      setParams({ run: tentativa.id });
      queryClient.invalidateQueries({ queryKey: TENTATIVAS_QUERY_KEY });
    } finally {
      setInitiating(false);
    }
  };

  const handlePararRun = () => {
    if (runId) enviarComando(runId, 'stop').catch(() => undefined);
  };

  return (
    <main className="telemetry-app">
      <aside className="telemetry-sidebar">
        <div className="brand">
          <div className="brand-mark">u</div>
          <div>
            <strong>MicroMouse</strong>
            <span>telemetry - v1.2</span>
          </div>
        </div>

        <label className="search-box">
          <span>⌕</span>
          <input aria-label="Buscar" placeholder="Buscar..." />
          <kbd>Ctrl K</kbd>
        </label>

        <nav aria-label="Monitoramento" className="side-nav">
          <span className="nav-label">Monitoramento</span>
          <button className={activeView === 'dashboard' ? 'active' : ''} onClick={() => setActiveView('dashboard')}>
            <span>▦</span> Dashboard
          </button>
          <button className={activeView === 'maze' ? 'active' : ''} onClick={() => setActiveView('maze')}>
            <span>▣</span> Maze
          </button>
          <button className={activeView === 'sensors' ? 'active' : ''} onClick={() => setActiveView('sensors')}>
            <span>⌁</span> Sensors
          </button>
          <button className={activeView === 'runs' ? 'active' : ''} onClick={() => setActiveView('runs')}>
            <span>◷</span> Runs {totalRuns > 0 && <small>{totalRuns}</small>}
          </button>
          {runId && (
            <button className={activeView === 'run' ? 'active indented' : 'indented'} onClick={() => setActiveView('run')}>
              Run - #{runId.slice(0, 6).toUpperCase()}
            </button>
          )}
        </nav>

        <div className={disconnected ? 'connection-card offline' : 'connection-card'}>
          <span className="wifi-dot">⌁</span>
          <div>
            <strong>Telemetria</strong>
            <span>{connected ? 'stream ativo (SSE)' : 'aguardando stream'}</span>
          </div>
        </div>

        <button className="ghost-button" onClick={logout}>
          Sair
        </button>
      </aside>

      <section className="telemetry-main">
        <header className="topbar">
          <div>
            <p className="breadcrumb">
              {real ? (tentativasQuery.data?.find((t) => t.id === runId)?.micromouse_nome ?? 'Mouse') : 'Micromouse'} &gt; {viewTitle(activeView)}
            </p>
            <div className="title-row">
              <h1>{viewHeading(activeView, telemetry.dimensao)}</h1>
              <span className={disconnected ? 'status-pill danger' : 'status-pill'}>
                <i /> {statusLabel}
              </span>
            </div>
            <p className="subtitle">
              {disconnected
                ? 'Perda de sinal detectada em menos de 2s'
                : `Visão geral da run atual - atualizado há ${Math.max(0, Math.round((now - lastPacketAt) / 1000))}s`}
            </p>
          </div>
          <div className="actions">
            <button
              className="ghost-button"
              disabled={!isRunning}
              onClick={handlePararRun}
            >
              ■ Parar run
            </button>
            <button
              className="primary-button"
              disabled={isRunning || initiating}
              onClick={handleIniciarRun}
            >
              {initiating ? '...' : '▶ Iniciar run'}
            </button>
          </div>
        </header>

        {activeView === 'dashboard' && (
          <DashboardView
            telemetry={telemetry}
            exploredPercent={exploredPercent}
            recentRuns={recentRuns}
            totalRuns={totalRuns}
            onOpenMaze={() => setActiveView('maze')}
            onOpenRuns={() => setActiveView('runs')}
          />
        )}
        {activeView === 'maze' && <MazeView telemetry={telemetry} exploredPercent={exploredPercent} />}
        {activeView === 'sensors' && <SensorsView telemetry={telemetry} />}
        {activeView === 'runs' && <RunsView rows={allRuns} runsEmCurso={runsEmCurso} onOpenRun={() => setActiveView('run')} />}
        {activeView === 'run' && <RunDetailView telemetry={telemetry} exploredPercent={exploredPercent} />}
      </section>
    </main>
  );
}

function viewTitle(view: TelemetryView) {
  return (
    {
      dashboard: 'Dashboard',
      maze: 'Labirinto',
      sensors: 'Sensores',
      runs: 'Runs',
      run: 'Runs > Run atual',
    } as Record<TelemetryView, string>
  )[view];
}

function viewHeading(view: TelemetryView, dimensao = 16) {
  return (
    {
      dashboard: 'Dashboard',
      maze: `Labirinto - ${dimensao}x${dimensao}`,
      sensors: 'Sensores',
      runs: 'Histórico de runs',
      run: 'Run atual',
    } as Record<TelemetryView, string>
  )[view];
}

function DashboardView({
  telemetry,
  exploredPercent,
  recentRuns,
  totalRuns,
  onOpenMaze,
  onOpenRuns,
}: {
  telemetry: TelemetrySnapshot;
  exploredPercent: string;
  recentRuns: RunRow[];
  totalRuns: number;
  onOpenMaze: () => void;
  onOpenRuns: () => void;
}) {
  return (
    <div className="view-stack">
      <MetricGrid telemetry={telemetry} exploredPercent={exploredPercent} compact={false} />
      <div className="dashboard-grid">
        <Panel
          className="maze-panel"
          title="Labirinto"
          subtitle={`Posição atual [${telemetry.position.join(',')}] - heading ${telemetry.heading}`}
          action={<button onClick={onOpenMaze}>→ Ver completo</button>}
        >
          <MazeCanvas telemetry={telemetry} mode="compact" />
        </Panel>
        <div className="side-panels">
          <SensorPanel telemetry={telemetry} />
          <SpeedChart telemetry={telemetry} />
        </div>
      </div>
      <Panel
        title="Runs recentes"
        action={<button onClick={onOpenRuns}>→ Ver todas ({totalRuns})</button>}
      >
        <RunsTable rows={recentRuns} compact />
      </Panel>
    </div>
  );
}

function MazeView({ telemetry, exploredPercent }: { telemetry: TelemetrySnapshot; exploredPercent: string }) {
  return (
    <div className="maze-view">
      <Panel
        className="maze-full"
        title="Mapa explorado"
        subtitle="Heatmap mostra recência da visita"
        action={
          <div className="segmented">
            <button>Caminho</button>
            <button className="active">Heatmap</button>
            <button>Paredes</button>
          </div>
        }
      >
        <MazeCanvas telemetry={telemetry} mode="full" />
      </Panel>
      <Panel title="Estado" className="state-panel">
        <div className="state-grid">
          <Stat label="Posição" value={`[${telemetry.position.join(',')}]`} />
          <Stat label="Heading" value={telemetry.heading} />
          <Stat label="Células" value={`${telemetry.exploredCells}/${telemetry.dimensao * telemetry.dimensao}`} helper={`${exploredPercent}%`} />
          <Stat label="Paredes" value={String(telemetry.walls)} />
          <Stat label="Tempo" value={formatDuration(telemetry.elapsedSeconds)} />
          <Stat label="Velocidade" value={`${telemetry.speed.toFixed(2)} m/s`} />
        </div>
      </Panel>
    </div>
  );
}

function SensorsView({ telemetry }: { telemetry: TelemetrySnapshot }) {
  return (
    <div className="view-stack">
      <div className="sensor-card-grid">
        {telemetry.sensors.map((sensor) => (
          <Panel
            key={sensor.label}
            title={`Sensor ${sensor.label.toLowerCase()}`}
            action={<span className={sensor.status === 'Livre' ? 'badge ok' : 'badge danger'}>{sensor.status}</span>}
          >
            <strong className="sensor-value">{sensor.value.toFixed(1)} <span>cm</span></strong>
            <MiniSparkline values={sensor.label === 'Frente' ? telemetry.frontHistory : telemetry.history} danger={sensor.status === 'Parede'} />
          </Panel>
        ))}
      </div>
      <div className="sensors-grid">
        <Panel title="Histograma - distribuição de leituras" subtitle="L - F - R combinados - janela 60s">
          <div className="histogram">
            {[6, 11, 18, 28, 42, 55, 40, 29, 22, 15, 9, 5].map((value, index) => (
              <span key={index} style={{ height: `${value}%` }}>
                <small>{value}</small>
              </span>
            ))}
          </div>
          <div className="axis-row"><span>0 cm</span><span>15 cm</span><span>30 cm</span></div>
        </Panel>
        <Panel title="IMU - orientação">
          <div className="imu-grid">
            <Stat label="Roll" value={`${telemetry.imu.roll.toFixed(1)}°`} tone="purple" />
            <Stat label="Pitch" value={`${telemetry.imu.pitch.toFixed(1)}°`} tone="cyan" />
            <Stat label="Yaw" value={`${telemetry.imu.yaw.toFixed(1)}°`} tone="purple" />
            <Stat label="Bias" value={telemetry.imu.bias.toFixed(2)} />
          </div>
        </Panel>
      </div>
      <Panel
        title="Stream de eventos"
        subtitle="WebSocket - ws://192.168.4.1:9001/stream"
        action={<span className="badge ok">Live</span>}
      >
        <EventStream telemetry={telemetry} />
      </Panel>
    </div>
  );
}

function RunsView({
  rows,
  runsEmCurso,
  onOpenRun,
}: {
  rows: RunRow[];
  runsEmCurso: number;
  onOpenRun: () => void;
}) {
  const finalizadas = rows.filter((r) => r.status === 'Finalizada').length;
  const abortadas = rows.filter((r) => r.status === 'Abortada').length;
  return (
    <div className="view-stack">
      <div className="runs-toolbar">
        <div className="tabs">
          <button className="active">Todos <span>{rows.length}</span></button>
          <button>Em curso <span>{runsEmCurso}</span></button>
          <button>Finalizadas <span>{finalizadas}</span></button>
          <button>Abortadas <span>{abortadas}</span></button>
        </div>
        <label className="table-search">
          ⌕ <input placeholder="Buscar por ID, mouse..." />
        </label>
      </div>
      <Panel>
        <RunsTable rows={rows} onOpenRun={onOpenRun} />
      </Panel>
      {rows.length > 0 && (
        <div className="pagination-row">
          <span>Mostrando 1-{rows.length} de {rows.length}</span>
        </div>
      )}
    </div>
  );
}

function RunDetailView({ telemetry, exploredPercent }: { telemetry: TelemetrySnapshot; exploredPercent: string }) {
  return (
    <div className="view-stack">
      <MetricGrid telemetry={telemetry} exploredPercent={exploredPercent} compact />
      <div className="run-detail-grid">
        <Panel className="maze-panel" title="Trajetória" subtitle="Visualização do caminho explorado">
          <MazeCanvas telemetry={telemetry} mode="compact" />
        </Panel>
        <div className="side-panels">
          <SpeedChart telemetry={telemetry} compare />
          <Panel title="Comparação">
            <ComparisonTable />
          </Panel>
        </div>
      </div>
      <Panel title="Replay" action={<span className="time-chip">04:32 / 12:40</span>}>
        <div className="replay">
          <button>↶</button>
          <button className="primary-button">▶</button>
          <button>■</button>
          <input aria-label="Replay" type="range" min="0" max="760" defaultValue="272" />
          <span>1x</span>
        </div>
      </Panel>
    </div>
  );
}

function MetricGrid({
  telemetry,
  exploredPercent,
  compact,
}: {
  telemetry: TelemetrySnapshot;
  exploredPercent: string;
  compact: boolean;
}) {
  const metrics = [
    { label: compact ? 'Duração' : 'Velocidade média', value: compact ? formatDuration(telemetry.elapsedSeconds) : telemetry.speed.toFixed(2), unit: compact ? 'rodando' : 'm/s', icon: '~' },
    { label: compact ? 'Vel. média' : 'Tempo decorrido', value: compact ? telemetry.speed.toFixed(2) : formatDuration(telemetry.elapsedSeconds), unit: compact ? 'm/s' : '', icon: '◷' },
    { label: compact ? 'Pico' : 'Bateria', value: compact ? telemetry.peakSpeed.toFixed(2) : Math.round(telemetry.battery).toString(), unit: compact ? 'm/s' : '%', icon: compact ? '~' : '▰' },
    { label: compact ? 'Células' : 'Tensão', value: compact ? String(telemetry.exploredCells) : telemetry.voltage.toFixed(2), unit: compact ? `/${telemetry.dimensao * telemetry.dimensao}` : 'V', icon: compact ? '▦' : 'V' },
    { label: compact ? 'Bateria usada' : 'Células exploradas', value: compact ? String(Math.round(100 - telemetry.battery)) : String(telemetry.exploredCells), unit: compact ? '%' : `/${telemetry.dimensao * telemetry.dimensao}`, icon: compact ? '▰' : '▦' },
  ];

  return (
    <div className={compact ? 'metric-grid compact' : 'metric-grid'}>
      {metrics.map((metric) => (
        <article className="metric-card" key={metric.label}>
          <div className="metric-heading">
            <span>{metric.icon}</span>
            <p>{metric.label}</p>
          </div>
          <strong>{metric.value} <small>{metric.unit}</small></strong>
          <em>{metric.label.includes('Células') ? `${exploredPercent}% explorado` : metric.label === 'Bateria' ? 'est. 18 min restantes' : ''}</em>
        </article>
      ))}
    </div>
  );
}

function Panel({
  title,
  subtitle,
  action,
  className = '',
  children,
}: {
  title?: string;
  subtitle?: string;
  action?: ReactNode;
  className?: string;
  children?: ReactNode;
}) {
  return (
    <section className={`panel ${className}`}>
      {(title || action) && (
        <header>
          <div>
            {title && <h2>{title}</h2>}
            {subtitle && <p>{subtitle}</p>}
          </div>
          {action}
        </header>
      )}
      <div className="panel-body">{children}</div>
    </section>
  );
}

function SensorPanel({ telemetry }: { telemetry: TelemetrySnapshot }) {
  return (
    <Panel title="Sensores - distância" action={<button>→ Detalhes</button>}>
      <div className="sensor-list">
        {telemetry.sensors.map((sensor) => (
          <div key={sensor.label}>
            <strong>{sensor.label}</strong>
            <span className="sensor-track">
              <i
                className={sensor.status === 'Parede' ? 'danger' : ''}
                style={{ width: `${Math.min(100, (sensor.value / 24) * 100)}%` }}
              />
            </span>
            <code>{sensor.value.toFixed(1)} cm</code>
            <em className={sensor.status === 'Livre' ? 'ok' : 'danger'}>{sensor.status === 'Livre' ? 'Clear' : 'Wall'}</em>
          </div>
        ))}
      </div>
    </Panel>
  );
}

function SpeedChart({ telemetry, compare = false }: { telemetry: TelemetrySnapshot; compare?: boolean }) {
  return (
    <Panel title={compare ? 'Velocidade ao longo do tempo' : 'Velocidade - últimos 60s'}>
      <MiniSparkline values={telemetry.history} large />
      {compare && <MiniSparkline values={telemetry.history.map((value) => value * 0.86)} large dashed />}
      <div className="chart-legend">
        <span>-60s</span>
        <span>-30s</span>
        <span>now</span>
      </div>
      {compare && <p className="legend-line"><i /> Atual <i className="cyan" /> Best run #022</p>}
    </Panel>
  );
}

function MiniSparkline({
  values,
  danger,
  large,
  dashed,
}: {
  values: number[];
  danger?: boolean;
  large?: boolean;
  dashed?: boolean;
}) {
  const points = buildSparkline(values, 320, large ? 112 : 78);
  const baseline = large ? '112' : '78';
  const color = danger ? '#ef2d24' : '#5145f5';
  const gradientId = danger ? 'dangerFill' : 'speedFill';

  return (
    <svg className={large ? 'sparkline large' : 'sparkline'} viewBox={`0 0 320 ${baseline}`} role="img" aria-label="Histórico de leituras">
      <defs>
        <linearGradient id={gradientId} x1="0" x2="0" y1="0" y2="1">
          <stop offset="0%" stopColor={color} stopOpacity="0.2" />
          <stop offset="100%" stopColor={color} stopOpacity="0" />
        </linearGradient>
      </defs>
      <polyline
        points={`0,${baseline} ${points} 320,${baseline}`}
        fill={`url(#${gradientId})`}
        stroke="none"
      />
      <polyline
        points={points}
        fill="none"
        stroke={dashed ? '#11a9d7' : color}
        strokeDasharray={dashed ? '5 5' : undefined}
        strokeLinecap="round"
        strokeWidth="3"
      />
    </svg>
  );
}

function mouseArrow(px: number, py: number, cell: number, heading: Heading): string {
  const cx = px + cell / 2;
  const cy = py + cell / 2;
  const r = cell * 0.3;
  const pts: Record<Heading, [number, number][]> = {
    Norte: [[cx, cy - r], [cx - r, cy + r], [cx + r, cy + r]],
    Sul: [[cx, cy + r], [cx - r, cy - r], [cx + r, cy - r]],
    Leste: [[cx + r, cy], [cx - r, cy - r], [cx - r, cy + r]],
    Oeste: [[cx - r, cy], [cx + r, cy - r], [cx + r, cy + r]],
  };
  return pts[heading].map(([x, y]) => `${x.toFixed(2)},${y.toFixed(2)}`).join(' ');
}

/**
 * Renderiza o labirinto N×N a partir do estado real vindo do backend:
 * `telemetry.maze` (paredes por célula), `telemetry.dimensao` e a pose.
 * A grade clara mostra as células ainda não descobertas.
 */
function MazeCanvas({ telemetry, mode }: { telemetry: TelemetrySnapshot; mode: 'compact' | 'full' }) {
  const n = Math.max(1, telemetry.dimensao);
  const cell = 100 / n;
  const stroke = Math.max(0.4, cell * 0.06);
  const goalSize = n % 2 === 0 ? 2 : 1;
  const goalMin = Math.floor((n - goalSize) / 2);
  const cells = Object.entries(telemetry.maze);
  const [px, py] = telemetry.position;

  return (
    <div className={mode === 'full' ? 'maze-canvas full' : 'maze-canvas'}>
      <svg viewBox="0 0 100 100" aria-label={`Mapa do labirinto ${n} por ${n}, rato em verde e chegada em azul`}>
        <rect width="100" height="100" fill="#fbfcff" />

        {/* Grade completa (clara) — células ainda por descobrir */}
        {Array.from({ length: n + 1 }, (_, i) => (
          <g key={`grid-${i}`}>
            <line x1={i * cell} y1={0} x2={i * cell} y2={100} stroke="#e6eaf2" strokeWidth={0.3} />
            <line x1={0} y1={i * cell} x2={100} y2={i * cell} stroke="#e6eaf2" strokeWidth={0.3} />
          </g>
        ))}

        {/* Células exploradas */}
        {cells.map(([key]) => {
          const [x, y] = key.split(',').map(Number);
          return <rect key={`cell-${key}`} x={x * cell} y={y * cell} width={cell} height={cell} fill="#5548ea" opacity={0.1} />;
        })}

        {/* Chegada (centro) */}
        <rect x={goalMin * cell} y={goalMin * cell} width={cell * goalSize} height={cell * goalSize} fill="#5145e8" opacity={0.55} />

        {/* Paredes por célula */}
        {cells.map(([key, walls]) => {
          const [x, y] = key.split(',').map(Number);
          const cx = x * cell;
          const cy = y * cell;
          const segs: [number, number, number, number][] = [];
          if (walls.n) segs.push([cx, cy, cx + cell, cy]);
          if (walls.s) segs.push([cx, cy + cell, cx + cell, cy + cell]);
          if (walls.w) segs.push([cx, cy, cx, cy + cell]);
          if (walls.e) segs.push([cx + cell, cy, cx + cell, cy + cell]);
          return segs.map(([x1, y1, x2, y2], idx) => (
            <line key={`wall-${key}-${idx}`} x1={x1} y1={y1} x2={x2} y2={y2} stroke="#1d2636" strokeWidth={stroke} strokeLinecap="round" />
          ));
        })}

        {/* Rato (verde, apontando conforme o heading) */}
        {telemetry.mazePose && (
          <polygon points={mouseArrow(px * cell, py * cell, cell, telemetry.heading)} fill="#09a775" stroke="#101827" strokeWidth={stroke * 0.5} />
        )}

        {/* Borda externa */}
        <rect x={0} y={0} width={100} height={100} fill="none" stroke="#1d2636" strokeWidth={stroke} />
      </svg>
      <div className="maze-legend" aria-label="Legenda do mapa">
        <span><i className="mouse" /> Rato</span>
        <span><i className="goal" /> Final</span>
      </div>
    </div>
  );
}

function RunsTable({
  rows,
  compact,
  onOpenRun,
}: {
  rows: RunRow[];
  compact?: boolean;
  onOpenRun?: () => void;
}) {
  return (
    <div className="table-wrap">
      <table>
        <thead>
          <tr>
            <th>ID</th>
            <th>Mouse</th>
            {!compact && <th>Algoritmo</th>}
            <th>Início</th>
            <th>Duração</th>
            <th>Vel. média</th>
            <th>Células</th>
            <th>Status</th>
            <th />
          </tr>
        </thead>
        <tbody>
          {rows.map((run) => (
            <tr key={run.id}>
              <td><strong className="run-id">{run.id}</strong></td>
              <td>{run.mouse}</td>
              {!compact && <td><span className="tag">{run.algorithm}</span></td>}
              <td>{run.start}</td>
              <td><code>{run.duration}</code></td>
              <td><code>{run.averageSpeed.toFixed(2)} m/s</code></td>
              <td>{run.cells}</td>
              <td><span className={`status-tag ${statusClass(run.status)}`}>{run.status}</span></td>
              <td><button onClick={onOpenRun}>Abrir →</button></td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

function EventStream({ telemetry }: { telemetry: TelemetrySnapshot }) {
  return (
    <div className="event-stream">
      {telemetry.events.map((event) => (
        <div key={`${event.time}-${event.name}`}>
          <code>{event.time}</code>
          <span className={`event-type ${event.type.toLowerCase()}`}>{event.type}</span>
          <strong>{event.name}</strong>
          <span>{event.detail}</span>
        </div>
      ))}
    </div>
  );
}

function ComparisonTable() {
  return (
    <div className="comparison-table">
      {[
        ['Tempo até centro', '02:14', '02:08', '+'],
        ['Erro de heading', '1.2°', '0.8°', '+'],
        ['Colisões', '0', '0', '='],
        ['Recalibrações', '3', '2', '+'],
      ].map(([label, current, best, delta]) => (
        <div key={label}>
          <span>{label}</span>
          <strong>{current}</strong>
          <code>{best}</code>
          <em className={delta === '=' ? 'ok' : ''}>{delta}</em>
        </div>
      ))}
    </div>
  );
}

function Stat({
  label,
  value,
  helper,
  tone,
}: {
  label: string;
  value: string;
  helper?: string;
  tone?: 'purple' | 'cyan';
}) {
  return (
    <div className={`stat ${tone ?? ''}`}>
      <span>{label}</span>
      <strong>{value}</strong>
      {helper && <small>{helper}</small>}
    </div>
  );
}

function statusClass(status: RunStatus) {
  return {
    'Em curso': 'running',
    Finalizada: 'done',
    Abortada: 'aborted',
  }[status];
}
