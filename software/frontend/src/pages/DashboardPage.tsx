import { useEffect, useMemo, useState, type ReactNode } from 'react';
import { useSearchParams } from 'react-router-dom';
import { useLogout } from '@/domains/auth';
import {
  enviarComando,
  useRunSnapshot,
  useTelemetryStream,
  useTentativas,
  type MazeState,
  type Pose,
  type RunSnapshot,
} from '@/domains/runs';
import './DashboardPage.css';

type TelemetryView = 'dashboard' | 'maze' | 'sensors' | 'runs' | 'run';
type Heading = 'Norte' | 'Leste' | 'Sul' | 'Oeste';

// Mapeia o heading do backend (N/S/E/W) para o rótulo em PT exibido na UI.
const HEADING_PT: Record<string, Heading> = { N: 'Norte', S: 'Sul', E: 'Leste', W: 'Oeste' };
const STATUS_PT: Record<string, string> = {
  em_curso: 'Run em andamento',
  finalizada: 'Finalizada',
  abortada: 'Abortada',
};
type RunStatus = 'Em curso' | 'Finalizada' | 'Abortada';

interface SensorReading {
  label: 'Esquerda' | 'Frente' | 'Direita';
  value: number;
  status: 'Livre' | 'Parede';
}

interface RunRow {
  id: string;
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
  // ── Campos reais (vêm da telemetria MQTT): o labirinto e a pose ──
  maze: MazeState;
  dimensao: number;
  mazePose: Pose | null;
}

const totalCells = 256;

const exploredPath = [
  [0, 15],
  [0, 14],
  [0, 13],
  [1, 13],
  [2, 13],
  [2, 12],
  [3, 12],
  [4, 12],
  [4, 11],
  [5, 11],
  [6, 11],
  [7, 11],
  [7, 10],
  [7, 9],
  [8, 9],
  [9, 9],
  [9, 8],
] as const;

const runs: RunRow[] = [
  {
    id: '#023',
    mouse: 'Mouse-01',
    algorithm: 'Flood Fill',
    start: '11/05 10:42',
    duration: '00:12:40',
    averageSpeed: 0.34,
    cells: 28,
    status: 'Em curso',
  },
  {
    id: '#022',
    mouse: 'Mouse-01',
    algorithm: 'Flood Fill',
    start: '11/05 10:24',
    duration: '00:18:22',
    averageSpeed: 0.28,
    cells: 256,
    status: 'Finalizada',
  },
  {
    id: '#021',
    mouse: 'Mouse-01',
    algorithm: 'Wall Follow',
    start: '11/05 09:58',
    duration: '00:09:14',
    averageSpeed: 0.31,
    cells: 142,
    status: 'Abortada',
  },
  {
    id: '#020',
    mouse: 'Mouse-02',
    algorithm: 'A*',
    start: '11/05 09:30',
    duration: '00:21:03',
    averageSpeed: 0.26,
    cells: 256,
    status: 'Finalizada',
  },
  {
    id: '#019',
    mouse: 'Mouse-02',
    algorithm: 'A*',
    start: '11/05 08:55',
    duration: '00:19:48',
    averageSpeed: 0.27,
    cells: 256,
    status: 'Finalizada',
  },
  {
    id: '#018',
    mouse: 'Mouse-01',
    algorithm: 'DFS',
    start: '10/05 22:14',
    duration: '00:14:55',
    averageSpeed: 0.3,
    cells: 244,
    status: 'Abortada',
  },
];

const initialTelemetry: TelemetrySnapshot = {
  speed: 0.34,
  peakSpeed: 0.48,
  battery: 78,
  voltage: 7.42,
  elapsedSeconds: 760,
  exploredCells: 28,
  position: [8, 10],
  heading: 'Norte',
  walls: 142,
  sensors: [
    { label: 'Esquerda', value: 4.2, status: 'Parede' },
    { label: 'Frente', value: 18.6, status: 'Livre' },
    { label: 'Direita', value: 2.1, status: 'Parede' },
  ],
  imu: { roll: 1.2, pitch: -0.4, yaw: 0, bias: 0.02 },
  history: [0.19, 0.24, 0.27, 0.31, 0.34, 0.32, 0.36, 0.42, 0.38, 0.35, 0.39, 0.44],
  frontHistory: [13, 14, 15, 16, 17, 18, 17, 18, 19, 18, 17, 18],
  events: [
    { time: '10:42:40.012', type: 'EVT', name: 'cell.entered', detail: '[8,10]' },
    { time: '10:42:38.480', type: 'CMD', name: 'motor.forward', detail: '18 cm' },
    { time: '10:42:36.220', type: 'EVT', name: 'rotation.complete', detail: '-90 graus' },
    { time: '10:42:34.105', type: 'WRN', name: 'wall.detected', detail: 'front - 2.1 cm' },
  ],
  maze: {},
  dimensao: 16,
  mazePose: null,
};

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

function updateTelemetry(previous: TelemetrySnapshot): TelemetrySnapshot {
  const nextSpeed = Math.max(0.18, Math.min(0.52, previous.speed + (Math.random() - 0.42) * 0.045));
  const nextFront = Math.max(2, Math.min(24, previous.sensors[1].value + (Math.random() - 0.45) * 1.8));
  const nextLeft = Math.max(1.5, Math.min(12, previous.sensors[0].value + (Math.random() - 0.55) * 0.9));
  const nextRight = Math.max(1.5, Math.min(12, previous.sensors[2].value + (Math.random() - 0.48) * 0.8));
  const shouldMove = previous.elapsedSeconds % 6 === 0;
  const pathIndex = Math.min(
    exploredPath.length - 1,
    Math.floor((previous.exploredCells - 12) / 2),
  );
  const nextPosition = exploredPath[pathIndex] ?? previous.position;
  const eventTime = new Date().toLocaleTimeString('pt-BR', {
    hour12: false,
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit',
  });

  return {
    ...previous,
    speed: nextSpeed,
    peakSpeed: Math.max(previous.peakSpeed, nextSpeed),
    battery: Math.max(0, previous.battery - 0.015),
    voltage: Math.max(6.4, previous.voltage - 0.002),
    elapsedSeconds: previous.elapsedSeconds + 1,
    exploredCells: shouldMove ? Math.min(totalCells, previous.exploredCells + 1) : previous.exploredCells,
    position: [nextPosition[0], nextPosition[1]],
    sensors: [
      { label: 'Esquerda', value: nextLeft, status: nextLeft < 6 ? 'Parede' : 'Livre' },
      { label: 'Frente', value: nextFront, status: nextFront < 7 ? 'Parede' : 'Livre' },
      { label: 'Direita', value: nextRight, status: nextRight < 6 ? 'Parede' : 'Livre' },
    ],
    imu: {
      roll: previous.imu.roll + (Math.random() - 0.5) * 0.12,
      pitch: previous.imu.pitch + (Math.random() - 0.5) * 0.1,
      yaw: previous.imu.yaw + (Math.random() - 0.5) * 0.2,
      bias: previous.imu.bias,
    },
    history: [...previous.history.slice(1), nextSpeed],
    frontHistory: [...previous.frontHistory.slice(1), nextFront],
    events: [
      {
        time: `${eventTime}.${String(Math.floor(Math.random() * 900)).padStart(3, '0')}`,
        type: nextFront < 7 ? 'WRN' : 'EVT',
        name: nextFront < 7 ? 'wall.detected' : 'telemetry.tick',
        detail: nextFront < 7 ? `front - ${nextFront.toFixed(1)} cm` : `${nextSpeed.toFixed(2)} m/s`,
      },
      ...previous.events.slice(0, 5),
    ],
  };
}

export function DashboardPage() {
  const [activeView, setActiveView] = useState<TelemetryView>('dashboard');
  const [streaming, setStreaming] = useState(true);
  const [now, setNow] = useState(0);
  const [lastPacketAt, setLastPacketAt] = useState(0);
  const [sim, setSim] = useState<TelemetrySnapshot>(initialTelemetry);
  const logout = useLogout();

  // Run real: ?run=<id> na URL, ou a tentativa mais recente.
  const [params] = useSearchParams();
  const tentativasQuery = useTentativas();
  const runId = params.get('run') ?? tentativasQuery.data?.[0]?.id ?? null;
  const snapshotQuery = useRunSnapshot(runId);
  const { snapshot: live, connected } = useTelemetryStream(runId);
  const real: RunSnapshot | null = live ?? snapshotQuery.data ?? null;

  useEffect(() => {
    const clock = window.setInterval(() => setNow(Date.now()), 250);
    return () => window.clearInterval(clock);
  }, []);

  // Simulação cosmética: sensores/IMU/sparklines/eventos seguem simulados até o
  // firmware enviá-los. O labirinto, a pose e as métricas principais são REAIS
  // (sobrepostos abaixo a partir de `real`).
  useEffect(() => {
    if (!streaming) {
      return undefined;
    }

    const interval = window.setInterval(() => {
      setSim((current) => updateTelemetry(current));
      setLastPacketAt(Date.now());
    }, 500);

    return () => window.clearInterval(interval);
  }, [streaming]);

  const telemetry: TelemetrySnapshot = useMemo(() => {
    if (!real) return sim;
    const pose = real.pose;
    const walls = Object.values(real.maze ?? {}).reduce(
      (acc, w) => acc + Number(w.n) + Number(w.s) + Number(w.e) + Number(w.w),
      0,
    );
    return {
      ...sim,
      speed: real.speed ?? real.velocidade_media ?? sim.speed,
      battery: real.battery ?? sim.battery,
      voltage: real.voltage ?? sim.voltage,
      exploredCells: real.explored ?? sim.exploredCells,
      position: pose ? [pose.x, pose.y] : sim.position,
      heading: pose ? HEADING_PT[pose.heading] : sim.heading,
      walls,
      maze: real.maze ?? {},
      dimensao: real.dimensao ?? sim.dimensao,
      mazePose: pose,
    };
  }, [real, sim]);

  const disconnected = !connected;
  const statusLabel = real ? STATUS_PT[real.status] ?? real.status : streaming ? 'Run em andamento' : 'Aguardando sinal';
  const total = telemetry.dimensao * telemetry.dimensao;
  const exploredPercent = ((telemetry.exploredCells / total) * 100).toFixed(1);
  const recentRuns = useMemo(() => runs.slice(0, 4), []);

  const comando = (acao: 'start' | 'stop') => {
    if (runId) enviarComando(runId, acao).catch(() => undefined);
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
            <span>◷</span> Runs <small>24</small>
          </button>
          <button className={activeView === 'run' ? 'active indented' : 'indented'} onClick={() => setActiveView('run')}>
            Run - #023
          </button>
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
            <p className="breadcrumb">Mouse-01 &gt; {viewTitle(activeView)}</p>
            <div className="title-row">
              <h1>{viewHeading(activeView)}</h1>
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
              disabled={!runId}
              onClick={() => {
                setStreaming(false);
                comando('stop');
              }}
            >
              ■ Parar run
            </button>
            <button
              className="primary-button"
              disabled={!runId}
              onClick={() => {
                setStreaming(true);
                setLastPacketAt(Date.now());
                comando('start');
              }}
            >
              ▶ Iniciar run
            </button>
          </div>
        </header>

        {activeView === 'dashboard' && (
          <DashboardView
            telemetry={telemetry}
            exploredPercent={exploredPercent}
            recentRuns={recentRuns}
            onOpenMaze={() => setActiveView('maze')}
            onOpenRuns={() => setActiveView('runs')}
          />
        )}
        {activeView === 'maze' && <MazeView telemetry={telemetry} exploredPercent={exploredPercent} />}
        {activeView === 'sensors' && <SensorsView telemetry={telemetry} />}
        {activeView === 'runs' && <RunsView rows={runs} onOpenRun={() => setActiveView('run')} />}
        {activeView === 'run' && <RunDetailView telemetry={telemetry} exploredPercent={exploredPercent} />}
      </section>
    </main>
  );
}

function viewTitle(view: TelemetryView) {
  return {
    dashboard: 'Dashboard',
    maze: 'Labirinto',
    sensors: 'Sensores',
    runs: 'Runs',
    run: 'Runs > Run #023',
  }[view];
}

function viewHeading(view: TelemetryView) {
  return {
    dashboard: 'Dashboard',
    maze: 'Labirinto - 16x16',
    sensors: 'Sensores',
    runs: 'Histórico de runs',
    run: 'Run #023',
  }[view];
}

function DashboardView({
  telemetry,
  exploredPercent,
  recentRuns,
  onOpenMaze,
  onOpenRuns,
}: {
  telemetry: TelemetrySnapshot;
  exploredPercent: string;
  recentRuns: RunRow[];
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
        action={<button onClick={onOpenRuns}>→ Ver todas (24)</button>}
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
          <Stat label="Células" value={`${telemetry.exploredCells}/${totalCells}`} helper={`${exploredPercent}%`} />
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

function RunsView({ rows, onOpenRun }: { rows: RunRow[]; onOpenRun: () => void }) {
  return (
    <div className="view-stack">
      <div className="runs-toolbar">
        <div className="tabs">
          <button className="active">Todos <span>24</span></button>
          <button>Em curso <span>1</span></button>
          <button>Finalizadas <span>18</span></button>
          <button>Abortadas <span>5</span></button>
        </div>
        <label className="table-search">
          ⌕ <input placeholder="Buscar por ID, mouse..." />
        </label>
      </div>
      <Panel>
        <RunsTable rows={rows} onOpenRun={onOpenRun} />
      </Panel>
      <div className="pagination-row">
        <span>Mostrando 1-6 de 24</span>
        <div>
          <button>Anterior</button>
          <button>Próximo</button>
        </div>
      </div>
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
    { label: compact ? 'Células' : 'Tensão', value: compact ? String(telemetry.exploredCells) : telemetry.voltage.toFixed(2), unit: compact ? `/${totalCells}` : 'V', icon: compact ? '▦' : 'V' },
    { label: compact ? 'Bateria usada' : 'Células exploradas', value: compact ? String(Math.round(100 - telemetry.battery)) : String(telemetry.exploredCells), unit: compact ? '%' : `/${totalCells}`, icon: compact ? '▰' : '▦' },
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
              <td>{run.cells}/{totalCells}</td>
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
