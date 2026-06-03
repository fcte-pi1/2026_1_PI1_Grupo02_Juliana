import type { Heading, MazeState, Pose } from '../../types';

interface MazeCanvasProps {
  maze: MazeState;
  pose: Pose | null;
  dimensao: number;
  mode?: 'compact' | 'full';
}

const VIEW = 100;

/** Triângulo (rato) apontando conforme o heading, centrado na célula. */
function mousePath(px: number, py: number, cell: number, heading: Heading): string {
  const cx = px + cell / 2;
  const cy = py + cell / 2;
  const r = cell * 0.32;
  const pts: Record<Heading, [number, number][]> = {
    N: [[cx, cy - r], [cx - r, cy + r], [cx + r, cy + r]],
    S: [[cx, cy + r], [cx - r, cy - r], [cx + r, cy - r]],
    E: [[cx + r, cy], [cx - r, cy - r], [cx - r, cy + r]],
    W: [[cx - r, cy], [cx + r, cy - r], [cx + r, cy + r]],
  };
  return pts[heading].map(([x, y]) => `${x.toFixed(2)},${y.toFixed(2)}`).join(' ');
}

/**
 * Renderiza o labirinto N×N a partir do estado autoritativo vindo do backend.
 * Cada célula descoberta desenha as paredes (n/s/e/w) e é pintada como explorada.
 * Sem dados mock — tudo deriva de `maze`/`pose`/`dimensao`.
 */
export function MazeCanvas({ maze, pose, dimensao, mode = 'compact' }: MazeCanvasProps) {
  const n = Math.max(1, dimensao);
  const cell = VIEW / n;
  const stroke = Math.max(0.4, cell * 0.06);

  // Região de chegada (centro): 2x2 para N par, 1x1 para N ímpar.
  const goalSize = n % 2 === 0 ? 2 : 1;
  const goalMin = Math.floor((n - goalSize) / 2);

  const cells = Object.entries(maze);

  return (
    <div className={mode === 'full' ? 'maze-canvas full' : 'maze-canvas'}>
      <svg
        viewBox={`0 0 ${VIEW} ${VIEW}`}
        aria-label={`Mapa do labirinto ${n} por ${n}, rato em verde e chegada em azul`}
      >
        <rect width={VIEW} height={VIEW} fill="#fbfcff" />

        {/* Grade completa (clara) — mostra o labirinto N×N ainda por descobrir */}
        {Array.from({ length: n + 1 }, (_, i) => (
          <g key={`grid-${i}`}>
            <line x1={i * cell} y1={0} x2={i * cell} y2={VIEW} stroke="#e6eaf2" strokeWidth={0.3} />
            <line x1={0} y1={i * cell} x2={VIEW} y2={i * cell} stroke="#e6eaf2" strokeWidth={0.3} />
          </g>
        ))}

        {/* Células exploradas */}
        {cells.map(([key]) => {
          const [x, y] = key.split(',').map(Number);
          return (
            <rect
              key={`cell-${key}`}
              x={x * cell}
              y={y * cell}
              width={cell}
              height={cell}
              fill="#5548ea"
              opacity={0.1}
            />
          );
        })}

        {/* Chegada (centro) */}
        <rect
          x={goalMin * cell}
          y={goalMin * cell}
          width={cell * goalSize}
          height={cell * goalSize}
          fill="#5145e8"
          opacity={0.55}
        />

        {/* Paredes por célula */}
        {cells.map(([key, walls]) => {
          const [x, y] = key.split(',').map(Number);
          const px = x * cell;
          const py = y * cell;
          const lines: [number, number, number, number][] = [];
          if (walls.n) lines.push([px, py, px + cell, py]);
          if (walls.s) lines.push([px, py + cell, px + cell, py + cell]);
          if (walls.w) lines.push([px, py, px, py + cell]);
          if (walls.e) lines.push([px + cell, py, px + cell, py + cell]);
          return lines.map(([x1, y1, x2, y2], idx) => (
            <line
              key={`wall-${key}-${idx}`}
              x1={x1}
              y1={y1}
              x2={x2}
              y2={y2}
              stroke="#1d2636"
              strokeWidth={stroke}
              strokeLinecap="round"
            />
          ));
        })}

        {/* Rato */}
        {pose && (
          <polygon
            points={mousePath(pose.x * cell, pose.y * cell, cell, pose.heading)}
            fill="#09a775"
            stroke="#101827"
            strokeWidth={stroke * 0.5}
          />
        )}

        {/* Borda externa */}
        <rect
          x={0}
          y={0}
          width={VIEW}
          height={VIEW}
          fill="none"
          stroke="#1d2636"
          strokeWidth={stroke}
        />
      </svg>
    </div>
  );
}
