import { createElement, useEffect, useMemo, useRef, useState, type ReactNode } from 'react';
import { useSearchParams } from 'react-router-dom';
import { useQueryClient } from '@tanstack/react-query';
import { useLogout } from '@/domains/auth';
import {
  enviarComando,
  fetchTrajetoria,
  iniciarCorrida,
  useRunSnapshot,
  useTelemetryStream,
  useTentativas,
  type MazeState,
  type Pose,
  type RunSnapshot,
  type Tentativa,
  type TrajetoriaPonto,
} from '@/domains/runs';
import { TENTATIVAS_QUERY_KEY } from '@/domains/runs/hooks/useTentativas';
import './DashboardPage.css';

type TelemetryView = 'dashboard' | 'maze' | 'runs' | 'run';
type Heading = 'Norte' | 'Leste' | 'Sul' | 'Oeste';
type RunStatus = 'Em curso' | 'Finalizada' | 'Abortada';

const HEADING_PT: Record<string, Heading> = { N: 'Norte', S: 'Sul', E: 'Leste', W: 'Oeste' };
const STATUS_PT: Record<string, string> = {
  em_curso: 'Em andamento',
  finalizada: 'Finalizada',
  abortada: 'Abortada',
};
const STATUS_RUN_PT: Record<string, RunStatus> = {
  em_curso: 'Em curso',
  finalizada: 'Finalizada',
  abortada: 'Abortada',
};

interface RunRow {
  id: string;
  uuid: string;
  mouse: string;
  labirinto: string;
  dimensao: number;
  start: string;
  duration: string;
  averageSpeed: number;
  battery: number;
  status: RunStatus;
  sucesso: boolean | null;
}

/** Campos do snapshot que o dashboard precisa exibir — apenas requisitos do trabalho. */
interface TelemetrySnapshot {
  /** Velocidade instantânea (para sparkline). */
  speed: number;
  /** Velocidade média acumulada — requisito slide 9. */
  velocidadeMedia: number;
  /** Tensão da bateria LiPo em volts (6.0–8.4 V). */
  voltage: number | null;
  /** Nível atual de bateria (%). */
  battery: number;
  /** Nível de bateria no início da run (primeiro pacote recebido). */
  startBattery: number | null;
  /** Consumo efetivo = startBattery - battery — requisito slide 9. */
  consumoBateria: number;
  /** Tempo decorrido em segundos — requisito slide 9. */
  elapsedSeconds: number;
  /** Células exploradas. */
  exploredCells: number;
  /** Pose atual. */
  position: [number, number];
  heading: Heading;
  mazePose: Pose | null;
  /** Mapa de paredes descobertas — base do trajeto. */
  maze: MazeState;
  /** Dimensão N do labirinto N×N. */
  dimensao: number;
  /** Nome do labirinto — requisito "tipo do labirinto" slide 9. */
  labirintoNome: string;
  /** Desafio cumprido S/N — requisito slide 9. */
  sucesso: boolean | null;
  /** Trajetória percorrida (células visitadas em ordem) — requisito "trajeto" slide 9. */
  trajectory: { x: number; y: number }[];
}

function tentativaToRow(t: Tentativa): RunRow {
  const durationSec =
    t.tempo_inicio && t.tempo_fim
      ? Math.floor(
          (new Date(t.tempo_fim).getTime() - new Date(t.tempo_inicio).getTime()) / 1000,
        )
      : null;
  return {
    id: `#${t.id.slice(-6).toUpperCase()}`,
    uuid: t.id,
    mouse: t.micromouse_nome || 'Mouse',
    labirinto: t.labirinto_nome || `${t.dimensao}x${t.dimensao}`,
    dimensao: t.dimensao,
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
    battery: t.explored > 0 ? (100 - (t.explored / ((t.dimensao ?? 16) * (t.dimensao ?? 16))) * 100) : 100,
    status: STATUS_RUN_PT[t.status] ?? 'Em curso',
    sucesso: t.sucesso,
  };
}

function formatDuration(seconds: number) {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  return [h, m, s].map((u) => String(u).padStart(2, '0')).join(':');
}

function clamp(v: number, lo: number, hi: number) { return Math.max(lo, Math.min(hi, v)); }

export function DashboardPage() {
  const [activeView, setActiveView] = useState<TelemetryView>('dashboard');
  const [now, setNow] = useState(0);
  const [lastPacketAt, setLastPacketAt] = useState(0);
  const [initiating, setInitiating] = useState(false);
  const [selectedDimensao, setSelectedDimensao] = useState<4 | 8>(4);
  const [trajectory, setTrajectory] = useState<TrajetoriaPonto[]>([]);
  const trajectoryRef = useRef<TrajetoriaPonto[]>([]);
  const startBatteryRef = useRef<number | null>(null);
  const [startBattery, setStartBattery] = useState<number | null>(null);
  const logout = useLogout();
  const queryClient = useQueryClient();

  const [params, setParams] = useSearchParams();
  const tentativasQuery = useTentativas();
  const runId = params.get('run') ?? tentativasQuery.data?.[0]?.id ?? null;
  const snapshotQuery = useRunSnapshot(runId);
  const { snapshot: live, connected } = useTelemetryStream(runId);
  const real: RunSnapshot | null = live ?? snapshotQuery.data ?? null;

  useEffect(() => {
    const clock = window.setInterval(() => setNow(Date.now()), 250);
    return () => window.clearInterval(clock);
  }, []);

  const prevStatusRef = useRef<string | null>(null);
  useEffect(() => {
    if (!real) return;
    const st = real.status;
    if ((st === 'finalizada' || st === 'abortada') && prevStatusRef.current !== st) {
      queryClient.invalidateQueries({ queryKey: TENTATIVAS_QUERY_KEY });
    }
    prevStatusRef.current = st;
  }, [real?.status, queryClient]);

  const prevLiveRef = useRef<RunSnapshot | null>(null);
  useEffect(() => {
    if (!live || live === prevLiveRef.current) return;
    prevLiveRef.current = live;
    setLastPacketAt(Date.now());
    if (live.battery != null && startBatteryRef.current === null) {
      startBatteryRef.current = live.battery;
      setStartBattery(live.battery);
    }
  }, [live]);

  useEffect(() => {
    prevLiveRef.current = null;
    startBatteryRef.current = null;
    setStartBattery(null);
  }, [runId]);

  useEffect(() => {
    trajectoryRef.current = [];
    setTrajectory([]);
    if (!runId) return;
    fetchTrajetoria(runId)
      .then((pts) => {
        trajectoryRef.current = pts;
        setTrajectory([...pts]);
        // Para runs históricas: usa bateria do primeiro ponto como referência inicial
        if (pts.length > 0 && pts[0].battery != null && startBatteryRef.current === null) {
          startBatteryRef.current = pts[0].battery;
          setStartBattery(pts[0].battery);
        }
      })
      .catch(() => undefined);
  }, [runId]);

  useEffect(() => {
    if (!live?.pose) return;
    const { x, y } = live.pose;
    const last = trajectoryRef.current[trajectoryRef.current.length - 1];
    if (!last || last.x !== x || last.y !== y) {
      const next = [...trajectoryRef.current, { x, y }];
      trajectoryRef.current = next;
      setTrajectory(next);
    }
  }, [live]);

  const elapsedSeconds = useMemo(() => {
    if (!real?.tempo_inicio) return 0;
    if (real.status === 'finalizada' && real.tempo_fim) {
      return Math.max(
        0,
        Math.floor((new Date(real.tempo_fim).getTime() - new Date(real.tempo_inicio).getTime()) / 1000),
      );
    }
    return Math.max(0, Math.floor((now - new Date(real.tempo_inicio).getTime()) / 1000));
  }, [real, now]);

  const currentTentativa = tentativasQuery.data?.find((t) => t.id === runId) ?? null;

  const telemetry: TelemetrySnapshot = useMemo(() => {
    const pose = real?.pose ?? null;
    const currentBattery = real?.battery ?? 100;
    const consumoBateria = startBattery !== null
      ? Math.max(0, startBattery - currentBattery)
      : 0;
    return {
      speed: real?.speed ?? 0,
      velocidadeMedia: real?.velocidade_media ?? 0,
      voltage: real?.voltage ?? null,
      battery: currentBattery,
      startBattery,
      consumoBateria,
      elapsedSeconds,
      exploredCells: real?.explored ?? 0,
      position: pose ? ([pose.x, pose.y] as [number, number]) : ([0, 0] as [number, number]),
      heading: pose ? (HEADING_PT[pose.heading] ?? 'Norte') : 'Norte',
      mazePose: pose,
      maze: real?.maze ?? {},
      dimensao: real?.dimensao ?? 16,
      labirintoNome: currentTentativa?.labirinto_nome ?? `${real?.dimensao ?? 16}×${real?.dimensao ?? 16}`,
      sucesso: real?.sucesso ?? currentTentativa?.sucesso ?? null,
      trajectory,
    };
  }, [real, elapsedSeconds, currentTentativa, trajectory, startBattery]);

  const isRunning = real?.status === 'em_curso';
  const disconnected = !connected;
  const statusLabel = real ? (STATUS_PT[real.status] ?? real.status) : 'Aguardando sinal';
  const total = telemetry.dimensao * telemetry.dimensao;
  const exploredPercent = ((telemetry.exploredCells / total) * 100).toFixed(1);

  const recentRuns = useMemo(
    () => (tentativasQuery.data ?? []).slice(0, 5).map(tentativaToRow),
    [tentativasQuery.data],
  );
  const allRuns = useMemo(
    () => (tentativasQuery.data ?? []).map(tentativaToRow),
    [tentativasQuery.data],
  );
  const totalRuns = tentativasQuery.data?.length ?? 0;
  const runsEmCurso = tentativasQuery.data?.filter((t) => t.status === 'em_curso').length ?? 0;

  const labirintosDisponiveis = useMemo(() => {
    const names = [...new Set((tentativasQuery.data ?? []).map((t) => t.labirinto_nome || `${t.dimensao}x${t.dimensao}`))];
    return names.sort();
  }, [tentativasQuery.data]);

  const handleIniciarRun = async () => {
    if (initiating) return;
    setInitiating(true);
    try {
      const tentativa = await iniciarCorrida(selectedDimensao);
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
            <span>Telemetria</span>
          </div>
        </div>

        <nav aria-label="Navegação" className="side-nav">
          <span className="nav-label">Monitoramento</span>
          <button className={activeView === 'dashboard' ? 'active' : ''} onClick={() => setActiveView('dashboard')}>
            <span className="nav-icon">▦</span> Dashboard
          </button>
          <button className={activeView === 'maze' ? 'active' : ''} onClick={() => setActiveView('maze')}>
            <span className="nav-icon">▣</span> Labirinto
          </button>
          <button className={activeView === 'runs' ? 'active' : ''} onClick={() => setActiveView('runs')}>
            <span className="nav-icon">◷</span> Histórico
            {totalRuns > 0 && <span className="nav-badge">{totalRuns}</span>}
          </button>
          {runId && (
            <button className={activeView === 'run' ? 'active indented' : 'indented'} onClick={() => setActiveView('run')}>
              Run #{runId.slice(-6).toUpperCase()}
            </button>
          )}
        </nav>

        <div className={disconnected ? 'connection-card offline' : 'connection-card'}>
          <span className="conn-icon">⌁</span>
          <div>
            <strong>Telemetria</strong>
            <span>{connected ? 'SSE ativo' : 'sem sinal'}</span>
          </div>
        </div>

        <button className="btn-logout" onClick={logout}>↩ Sair</button>
      </aside>

      <section className="telemetry-main">
        <header className="topbar">
          <div>
            <p className="breadcrumb">
              {currentTentativa?.micromouse_nome ?? 'Micromouse'} &gt; {VIEW_TITLE[activeView]}
            </p>
            <div className="title-row">
              <h1>{activeView === 'maze' ? `Labirinto ${telemetry.labirintoNome}` : VIEW_TITLE[activeView]}</h1>
              <span className={disconnected ? 'status-pill danger' : 'status-pill'}>
                <i /> {statusLabel}
              </span>
            </div>
            <p className="subtitle">
              {disconnected
                ? 'Sem sinal do micromouse'
                : `Labirinto: ${telemetry.labirintoNome} — atualizado há ${Math.max(0, Math.round((now - lastPacketAt) / 1000))}s`}
            </p>
          </div>
          <div className="actions">
            <select
              className="btn-select"
              disabled={isRunning || initiating}
              value={selectedDimensao}
              onChange={(e) => setSelectedDimensao(Number(e.target.value) as 4 | 8)}
              aria-label="Tamanho do labirinto"
            >
              <option value={4}>Labirinto 4×4</option>
              <option value={8}>Labirinto 8×8</option>
            </select>
            <button className="btn-stop" disabled={!isRunning} onClick={handlePararRun}>
              Parar
            </button>
            <button className="btn-primary" disabled={isRunning || initiating} onClick={handleIniciarRun}>
              {initiating ? 'Iniciando…' : '▶ Iniciar run'}
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
        {activeView === 'runs' && (
          <RunsView
            rows={allRuns}
            runsEmCurso={runsEmCurso}
            labirintos={labirintosDisponiveis}
            onOpenRun={() => setActiveView('run')}
          />
        )}
        {activeView === 'run' && <RunDetailView telemetry={telemetry} exploredPercent={exploredPercent} />}
      </section>
    </main>
  );
}

const VIEW_TITLE: Record<TelemetryView, string> = {
  dashboard: 'Dashboard',
  maze: 'Labirinto',
  runs: 'Histórico de runs',
  run: 'Detalhes da run',
};

// ---------------------------------------------------------------------------
// Sub-views
// ---------------------------------------------------------------------------

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
      <MetricGrid telemetry={telemetry} exploredPercent={exploredPercent} />
      <div className="dashboard-grid">
        <Panel
          className="maze-panel"
          title="Trajeto no labirinto"
          subtitle={`Posição [${telemetry.position.join(',')}] · ${telemetry.heading}`}
          action={<button onClick={onOpenMaze}>→ Ver completo</button>}
        >
          <MazeCanvas telemetry={telemetry} mode="compact" />
        </Panel>
        <LiveDataPanel telemetry={telemetry} />
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
      <Panel className="maze-full" title="Trajeto no labirinto">
        <MazeCanvas telemetry={telemetry} mode="full" />
      </Panel>
      <Panel title="Dados da run" className="state-panel">
        <div className="state-grid">
          <Stat label="Tipo do labirinto" value={telemetry.labirintoNome} />
          <Stat label="Posição atual" value={`[${telemetry.position.join(',')}]`} />
          <Stat label="Direção" value={telemetry.heading} />
          <Stat label="Células exploradas" value={`${telemetry.exploredCells}/${telemetry.dimensao * telemetry.dimensao}`} helper={`${exploredPercent}%`} />
          <Stat label="Tempo decorrido" value={formatDuration(telemetry.elapsedSeconds)} />
          <Stat label="Velocidade média" value={`${telemetry.velocidadeMedia.toFixed(2)} m/s`} />
          <Stat
            label="Consumo de bateria"
            value={`${telemetry.consumoBateria.toFixed(1)}%`}
            helper={telemetry.startBattery !== null ? `${telemetry.battery.toFixed(0)}% restante` : undefined}
          />
          <Stat
            label="Desafio cumprido"
            value={telemetry.sucesso === true ? 'Sim' : telemetry.sucesso === false ? 'Não' : 'Em andamento'}
            tone={telemetry.sucesso === true ? 'success' : telemetry.sucesso === false ? 'danger' : undefined}
          />
        </div>
      </Panel>
    </div>
  );
}

function RunsView({
  rows,
  runsEmCurso,
  labirintos,
  onOpenRun,
}: {
  rows: RunRow[];
  runsEmCurso: number;
  labirintos: string[];
  onOpenRun: () => void;
}) {
  const [statusFilter, setStatusFilter] = useState<'todos' | 'em_curso' | 'finalizada' | 'abortada'>('todos');
  const [labirintoFilter, setLabirintoFilter] = useState<string>('todos');

  const filtered = rows.filter((r) => {
    const statusOk =
      statusFilter === 'todos' ||
      (statusFilter === 'em_curso' && r.status === 'Em curso') ||
      (statusFilter === 'finalizada' && r.status === 'Finalizada') ||
      (statusFilter === 'abortada' && r.status === 'Abortada');
    const labOk = labirintoFilter === 'todos' || r.labirinto === labirintoFilter;
    return statusOk && labOk;
  });

  const finalizadas = rows.filter((r) => r.status === 'Finalizada').length;
  const abortadas = rows.filter((r) => r.status === 'Abortada').length;

  return (
    <div className="view-stack">
      <div className="runs-toolbar">
        <div className="tabs">
          <button className={statusFilter === 'todos' ? 'active' : ''} onClick={() => setStatusFilter('todos')}>
            Todos <span>{rows.length}</span>
          </button>
          <button className={statusFilter === 'em_curso' ? 'active' : ''} onClick={() => setStatusFilter('em_curso')}>
            Em curso <span>{runsEmCurso}</span>
          </button>
          <button className={statusFilter === 'finalizada' ? 'active' : ''} onClick={() => setStatusFilter('finalizada')}>
            Finalizadas <span>{finalizadas}</span>
          </button>
          <button className={statusFilter === 'abortada' ? 'active' : ''} onClick={() => setStatusFilter('abortada')}>
            Abortadas <span>{abortadas}</span>
          </button>
        </div>
        <select
          className="ghost-button"
          value={labirintoFilter}
          onChange={(e) => setLabirintoFilter(e.target.value)}
          aria-label="Filtrar por labirinto"
        >
          <option value="todos">Todos os labirintos</option>
          {labirintos.map((lab) => (
            <option key={lab} value={lab}>{lab}</option>
          ))}
        </select>
      </div>
      <Panel>
        <RunsTable rows={filtered} onOpenRun={onOpenRun} />
      </Panel>
      {filtered.length > 0 && (
        <div className="pagination-row">
          <span>Mostrando {filtered.length} de {rows.length} runs</span>
        </div>
      )}
    </div>
  );
}

function RunDetailView({ telemetry, exploredPercent }: { telemetry: TelemetrySnapshot; exploredPercent: string }) {
  return (
    <div className="view-stack">
      <Panel title="Trajeto percorrido" subtitle="Caminho completo no labirinto">
        <MazeCanvas telemetry={telemetry} mode="full" />
      </Panel>
      <Panel title="Resultado da run">
        <div className="state-grid">
          <Stat label="Tipo do labirinto" value={telemetry.labirintoNome} />
          <Stat
            label="Desafio cumprido"
            value={telemetry.sucesso === true ? 'Sim ✓' : telemetry.sucesso === false ? 'Não ✗' : 'Em andamento'}
            tone={telemetry.sucesso === true ? 'success' : telemetry.sucesso === false ? 'danger' : undefined}
          />
          <Stat label="Tempo de conclusão" value={formatDuration(telemetry.elapsedSeconds)} />
          <Stat label="Velocidade média" value={`${telemetry.velocidadeMedia.toFixed(2)} m/s`} />
          <Stat
            label="Consumo de bateria"
            value={`${telemetry.consumoBateria.toFixed(1)}%`}
            helper={telemetry.startBattery !== null ? `${telemetry.battery.toFixed(0)}% restante` : undefined}
          />
          <Stat label="Células exploradas" value={`${telemetry.exploredCells}/${telemetry.dimensao * telemetry.dimensao}`} helper={`${exploredPercent}%`} />
        </div>
      </Panel>
    </div>
  );
}

// ---------------------------------------------------------------------------
// Componentes reutilizáveis
// ---------------------------------------------------------------------------

function MetricGrid({ telemetry, exploredPercent }: { telemetry: TelemetrySnapshot; exploredPercent: string }) {
  const sucesso = telemetry.sucesso;
  return (
    <div className="metric-grid">
      <article className="metric-card">
        <div className="metric-card-head">
          <span className="metric-icon indigo">
            <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
              <path d="M3.34 19a10 10 0 1 1 17.32 0" />
              <path d="m12 14 4-4" />
            </svg>
          </span>
          <span className="metric-label">Velocidade média</span>
        </div>
        <div className="metric-value">
          {telemetry.velocidadeMedia.toFixed(2)} <small>m/s</small>
        </div>
        <div className="metric-helper">velocidade instantânea: {telemetry.speed.toFixed(2)} m/s</div>
      </article>

      <article className="metric-card">
        <div className="metric-card-head">
          <span className="metric-icon blue">◷</span>
          <span className="metric-label">Tempo decorrido</span>
        </div>
        <div className="metric-value">{formatDuration(telemetry.elapsedSeconds)}</div>
        <div className="metric-helper">{exploredPercent}% do labirinto explorado</div>
      </article>

      <article className="metric-card">
        <div className="metric-card-head">
          <span className="metric-icon amber">▰</span>
          <span className="metric-label">Consumo de bateria</span>
        </div>
        <div className="metric-value">
          {telemetry.consumoBateria.toFixed(1)} <small>%</small>
        </div>
        <div className="metric-helper">
          {telemetry.startBattery !== null
            ? `${telemetry.battery.toFixed(0)}% restante`
            : 'aguardando primeiro pacote'}
        </div>
      </article>

      <article className="metric-card">
        <div className="metric-card-head">
          <span className={`metric-icon ${sucesso === true ? 'green' : sucesso === false ? 'red' : 'indigo'}`}>
            {sucesso === true ? '✓' : sucesso === false ? '✗' : '?'}
          </span>
          <span className="metric-label">Desafio cumprido</span>
        </div>
        <div className={`metric-value ${sucesso === true ? 'success' : sucesso === false ? 'danger' : ''}`}>
          {sucesso === true ? 'Sim' : sucesso === false ? 'Não' : 'Em andamento'}
        </div>
        <div className="metric-helper">
          {telemetry.labirintoNome}
        </div>
      </article>
    </div>
  );
}

function Panel({
  title,
  subtitle,
  action,
  className = '',
  centered = false,
  children,
}: {
  title?: string;
  subtitle?: string;
  action?: ReactNode;
  className?: string;
  centered?: boolean;
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
      <div className={`panel-body${centered ? ' centered' : ''}`}>{children}</div>
    </section>
  );
}

function LiveDataPanel({ telemetry }: { telemetry: TelemetrySnapshot }) {
  const total = telemetry.dimensao * telemetry.dimensao;
  const exploredPct = clamp((telemetry.exploredCells / total) * 100, 0, 100);
  const batteryPct = clamp(telemetry.battery, 0, 100);
  const [px, py] = telemetry.position;

  const HEADING_ARROW: Record<string, string> = { Norte: '↑', Sul: '↓', Leste: '→', Oeste: '←' };

  return (
    <Panel title="Ao vivo">
      <div className="live-panel">
        <div className="live-section">
          <span className="live-section-label">Velocidade atual</span>
          <div className="live-speed-value">
            {telemetry.speed.toFixed(2)}<small>m/s</small>
          </div>
          <span className="live-section-sub">média: {telemetry.velocidadeMedia.toFixed(2)} m/s</span>
        </div>

        <div className="live-section">
          <div className="live-bar-header">
            <span className="live-section-label">Exploração</span>
            <span className="live-bar-pct">{exploredPct.toFixed(1)}%</span>
          </div>
          <div className="live-bar-track">
            <div className="live-bar-fill indigo" style={{ width: `${exploredPct}%` }} />
          </div>
          <span className="live-section-sub">{telemetry.exploredCells} de {total} células</span>
        </div>

        <div className="live-section">
          <div className="live-bar-header">
            <span className="live-section-label">Bateria restante</span>
            <span className="live-bar-pct">{batteryPct.toFixed(1)}%</span>
          </div>
          <div className="live-bar-track">
            <div
              className={`live-bar-fill ${batteryPct > 40 ? 'green' : batteryPct > 20 ? 'amber' : 'red'}`}
              style={{ width: `${batteryPct}%` }}
            />
          </div>
          <span className="live-section-sub">consumido: {telemetry.consumoBateria.toFixed(1)}%</span>
        </div>

        {telemetry.voltage !== null && (
          <div className="live-section">
            <span className="live-section-label">Tensão da bateria</span>
            <div className="live-voltage-row">
              <span className="live-voltage-value">{telemetry.voltage.toFixed(2)}<small>V</small></span>
              <span className={`live-voltage-tag ${telemetry.voltage >= 7.8 ? 'ok' : telemetry.voltage >= 7.0 ? 'warn' : 'low'}`}>
                {telemetry.voltage >= 7.8 ? 'OK' : telemetry.voltage >= 7.0 ? 'Baixa' : 'Crítica'}
              </span>
            </div>
            <span className="live-section-sub">LiPo 2S · nominal 7.4 V · mín 6.0 V</span>
          </div>
        )}

        <div className="live-section">
          <span className="live-section-label">Posição atual</span>
          <div className="live-position">
            <span className="live-coords">[{px.toFixed(1)}, {py.toFixed(1)}]</span>
            <span className="live-heading">
              {HEADING_ARROW[telemetry.heading] ?? '–'} {telemetry.heading}
            </span>
          </div>
        </div>
      </div>
    </Panel>
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
      <svg viewBox="0 0 100 100" aria-label={`Labirinto ${n}×${n}`}>
        <rect width="100" height="100" fill="#fbfcff" />

        {/* Grade de fundo */}
        {Array.from({ length: n + 1 }, (_, i) => (
          <g key={`grid-${i}`}>
            <line x1={i * cell} y1={0} x2={i * cell} y2={100} stroke="#e6eaf2" strokeWidth={0.3} />
            <line x1={0} y1={i * cell} x2={100} y2={i * cell} stroke="#e6eaf2" strokeWidth={0.3} />
          </g>
        ))}

        {/* Células descobertas */}
        {cells.map(([key]) => {
          const [x, y] = key.split(',').map(Number);
          return <rect key={`cell-${key}`} x={x * cell} y={y * cell} width={cell} height={cell} fill="#5548ea" opacity={0.1} />;
        })}

        {/* Região de chegada (centro) */}
        <rect x={goalMin * cell} y={goalMin * cell} width={cell * goalSize} height={cell * goalSize} fill="#5145e8" opacity={0.55} />

        {/* Paredes descobertas */}
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

        {/* Trajeto percorrido (amarelo) */}
        {telemetry.trajectory.slice(0, -1).map(({ x, y }, idx) =>
          createElement('line', {
            key: `traj-${idx}`,
            x1: x * cell + cell / 2,
            y1: y * cell + cell / 2,
            x2: telemetry.trajectory[idx + 1].x * cell + cell / 2,
            y2: telemetry.trajectory[idx + 1].y * cell + cell / 2,
            stroke: '#f59e0b',
            strokeWidth: Math.max(0.4, cell * 0.18),
            strokeLinecap: 'round',
            opacity: 0.8,
          })
        )}

        {/* Ponto de partida */}
        {telemetry.trajectory.length > 0 &&
          createElement('rect', {
            x: telemetry.trajectory[0].x * cell + cell / 2 - Math.max(0.6, cell * 0.2),
            y: telemetry.trajectory[0].y * cell + cell / 2 - Math.max(0.6, cell * 0.2),
            width: Math.max(1.2, cell * 0.4),
            height: Math.max(1.2, cell * 0.4),
            fill: '#f59e0b',
            opacity: 0.9,
          })
        }

        {/* Rato */}
        {telemetry.mazePose && (
          <polygon
            points={mouseArrow(px * cell, py * cell, cell, telemetry.heading)}
            fill="#09a775"
            stroke="#101827"
            strokeWidth={stroke * 0.5}
          />
        )}

        {/* Borda */}
        <rect x={0} y={0} width={100} height={100} fill="none" stroke="#1d2636" strokeWidth={stroke} />
      </svg>
      <div className="maze-legend" aria-label="Legenda">
        <span><i className="mouse" /> Rato</span>
        <span><i className="goal" /> Chegada</span>
        <span><i className="trajeto" /> Trajeto</span>
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
  if (rows.length === 0) {
    return (
      <div style={{ padding: '32px', textAlign: 'center', color: 'var(--text-3)', fontSize: '13px' }}>
        Nenhuma run encontrada.
      </div>
    );
  }
  return (
    <div className="table-wrap">
      <table>
        <thead>
          <tr>
            <th>Run</th>
            {!compact && <th>Labirinto</th>}
            <th>Início</th>
            <th>Duração</th>
            <th>Vel. média</th>
            <th>Desafio</th>
            <th>Status</th>
            {onOpenRun && <th />}
          </tr>
        </thead>
        <tbody>
          {rows.map((run) => (
            <tr key={run.uuid}>
              <td><span className="run-id">{run.id}</span></td>
              {!compact && <td><span className="tag">{run.labirinto}</span></td>}
              <td style={{ color: 'var(--text-2)' }}>{run.start}</td>
              <td><code>{run.duration}</code></td>
              <td><code>{run.averageSpeed.toFixed(2)} m/s</code></td>
              <td>
                <span className={`desafio-tag ${run.sucesso === true ? 'yes' : run.sucesso === false ? 'no' : 'nd'}`}>
                  {run.sucesso === true ? '✓ Sim' : run.sucesso === false ? '✗ Não' : '—'}
                </span>
              </td>
              <td><span className={`status-tag ${statusClass(run.status)}`}>{run.status}</span></td>
              {onOpenRun && (
                <td>
                  <button className="btn-table-open" onClick={onOpenRun}>Ver detalhes</button>
                </td>
              )}
            </tr>
          ))}
        </tbody>
      </table>
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
  tone?: 'success' | 'danger' | 'muted';
}) {
  return (
    <div className="stat">
      <div className="stat-label">{label}</div>
      <div className={`stat-value${tone ? ` ${tone}` : ''}`}>{value}</div>
      {helper && <div className="stat-helper">{helper}</div>}
    </div>
  );
}

function statusClass(status: RunStatus) {
  return { 'Em curso': 'running', Finalizada: 'done', Abortada: 'aborted' }[status];
}
