export { MazeCanvas } from './components/MazeCanvas/MazeCanvas';
export { useTentativas } from './hooks/useTentativas';
export { useRunSnapshot } from './hooks/useRunSnapshot';
export { useTelemetryStream } from './hooks/useTelemetryStream';
export { enviarComando, fetchSnapshot, fetchTentativas, iniciarCorrida } from './api';
export type {
  Heading,
  MazeState,
  Pose,
  RunSnapshot,
  Tentativa,
  WallFlags,
} from './types';
