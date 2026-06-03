export type Heading = 'N' | 'S' | 'E' | 'W';

export interface WallFlags {
  n: boolean;
  s: boolean;
  e: boolean;
  w: boolean;
}

export interface Pose {
  x: number;
  y: number;
  heading: Heading;
}

/** Estado esparso do labirinto: chave "x,y" -> paredes da célula. */
export type MazeState = Record<string, WallFlags>;

/** Snapshot servido pelo backend (REST inicial + cada evento SSE). */
export interface RunSnapshot {
  tentativa_id: string;
  dimensao: number;
  status: string;
  pose: Pose | null;
  maze: MazeState;
  explored: number;
  velocidade_media: number | null;
  consumo_bateria: number | null;
  speed: number | null;
  battery: number | null;
  voltage: number | null;
  ts: string | null;
}

/** Item da listagem de tentativas (GET /v1/runs/tentativas/). */
export interface Tentativa {
  id: string;
  micromouse: string;
  labirinto: string;
  dimensao: number;
  status: string;
  sucesso: boolean | null;
  velocidade_media: number | null;
  tempo_inicio: string | null;
  tempo_fim: string | null;
  explored: number;
  created_at: string;
}
