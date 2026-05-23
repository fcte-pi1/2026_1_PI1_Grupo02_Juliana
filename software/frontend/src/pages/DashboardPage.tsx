import { useEffect, useMemo, useState, type ReactNode } from 'react';
import './DashboardPage.css';

type TelemetryView = 'dashboard' | 'maze' | 'sensors' | 'runs' | 'run';
type Heading = 'Norte' | 'Leste' | 'Sul' | 'Oeste';
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
}

const totalCells = 256;
const mazeWalls = [
  { x: 0, y: 0, w: 4, h: 2 },
  { x: 4, y: 0, w: 1, h: 4 },
  { x: 7, y: 0, w: 1, h: 2 },
  { x: 12, y: 3, w: 3, h: 1 },
  { x: 2, y: 5, w: 3, h: 1 },
  { x: 6, y: 4, w: 1, h: 2 },
  { x: 8, y: 5, w: 4, h: 1 },
  { x: 12, y: 5, w: 1, h: 3 },
  { x: 14, y: 6, w: 1, h: 2 },
  { x: 3, y: 7, w: 1, h: 3 },
  { x: 6, y: 8, w: 1, h: 3 },
  { x: 9, y: 7, w: 1, h: 4 },
  { x: 4, y: 11, w: 5, h: 1 },
  { x: 11, y: 10, w: 1, h: 3 },
  { x: 13, y: 12, w: 2, h: 1 },
  { x: 12, y: 14, w: 1, h: 2 },
];

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
const mazeGoalPosition: [number, number] = [7, 7];
const mazeGoalSize = 2;

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
  const [telemetry, setTelemetry] = useState<TelemetrySnapshot>(initialTelemetry);
  const disconnected = lastPacketAt > 0 && now - lastPacketAt > 2000;

  useEffect(() => {
    const clock = window.setInterval(() => setNow(Date.now()), 250);
    return () => window.clearInterval(clock);
  }, []);

  useEffect(() => {
    if (!streaming) {
      return undefined;
    }

    const interval = window.setInterval(() => {
      setTelemetry((current) => updateTelemetry(current));
      setLastPacketAt(Date.now());
    }, 500);

    return () => window.clearInterval(interval);
  }, [streaming]);

  const statusLabel = disconnected ? 'Desconectado' : streaming ? 'Run em andamento' : 'Aguardando sinal';
  const exploredPercent = ((telemetry.exploredCells / totalCells) * 100).toFixed(1);
  const recentRuns = useMemo(() => runs.slice(0, 4), []);

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
            <strong>Mouse-01</strong>
            <span>{disconnected ? 'sem sinal' : '192.168.4.1 - 42ms'}</span>
          </div>
          <button aria-label="Mais opções">...</button>
        </div>
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
            <button className="ghost-button" onClick={() => setStreaming(false)}>
              ■ Parar run
            </button>
            <button
              className="primary-button"
              onClick={() => {
                setStreaming(true);
                setLastPacketAt(Date.now());
              }}
            >
              ▶ Nova run
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

function MazeCanvas({ telemetry, mode }: { telemetry: TelemetrySnapshot; mode: 'compact' | 'full' }) {
  const cell = 100 / 16;
  const visiblePath = exploredPath.slice(0, Math.max(6, Math.min(exploredPath.length, telemetry.exploredCells - 10)));

  return (
    <div className={mode === 'full' ? 'maze-canvas full' : 'maze-canvas'}>
      <svg viewBox="0 0 100 100" aria-label="Mapa do labirinto 16 por 16 com rato em verde e área de chegada 2 por 2 em azul">
        <rect width="100" height="100" rx="0.4" fill="#fbfcff" />
        {visiblePath.map(([x, y], index) => (
          <rect
            key={`${x}-${y}`}
            x={x * cell}
            y={y * cell}
            width={cell}
            height={cell}
            fill="#5548ea"
            opacity={0.1 + (index / visiblePath.length) * 0.34}
          />
        ))}
        {mazeWalls.map((wall, index) => (
          <rect
            key={index}
            x={wall.x * cell}
            y={wall.y * cell}
            width={Math.max(0.32, wall.w * cell)}
            height={Math.max(0.32, wall.h * cell)}
            fill="#1d2636"
            opacity="0.86"
          />
        ))}
        <rect
          x={mazeGoalPosition[0] * cell}
          y={mazeGoalPosition[1] * cell}
          width={cell * mazeGoalSize}
          height={cell * mazeGoalSize}
          rx="0.08"
          fill="#5145e8"
        />
        <rect
          x={telemetry.position[0] * cell}
          y={telemetry.position[1] * cell}
          width={cell * 1.05}
          height={cell * 1.05}
          rx="0.08"
          fill="#09a775"
          stroke="#101827"
          strokeWidth="0.4"
        />
        <path
          d={`M ${(telemetry.position[0] + 0.52) * cell} ${(telemetry.position[1] + 0.28) * cell} l 1.45 1.45 l -1.45 1.45 l -1.45 -1.45 z`}
          fill="#ffffff"
        />
        <text x="0" y="103" fill="#94a0b6" fontSize="2.2">(0,0)</text>
        <text x="44" y="103" fill="#94a0b6" fontSize="2.2">16 x 16 cells</text>
        <text x="92" y="103" fill="#94a0b6" fontSize="2.2">(15,15)</text>
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
