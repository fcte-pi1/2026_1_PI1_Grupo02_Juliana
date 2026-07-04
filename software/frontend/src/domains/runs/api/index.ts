import { apiClient } from '@/domains/auth/api/client';
import type { PaginatedResponse } from '@/lib/types';
import type { RunSnapshot, Tentativa } from '../types';

/** Lista as tentativas (corridas). Interceptor do apiClient já desembrulha o envelope. */
export async function fetchTentativas(): Promise<Tentativa[]> {
  const { data } = await apiClient.get<PaginatedResponse<Tentativa>>(
    '/v1/runs/tentativas/',
  );
  return data.results;
}

/** Snapshot inicial do labirinto + pose + métricas de uma tentativa. */
export async function fetchSnapshot(runId: string): Promise<RunSnapshot> {
  const { data } = await apiClient.get<RunSnapshot>(
    `/v1/runs/tentativas/${runId}/snapshot/`,
  );
  return data;
}

/** Controle remoto (RF22): publica start/stop no MQTT via backend. */
export async function enviarComando(
  runId: string,
  acao: 'start' | 'stop',
): Promise<void> {
  await apiClient.post(`/v1/runs/tentativas/${runId}/comando/`, { acao });
}

/** Cria uma nova Tentativa no labirinto escolhido e inicia a corrida. */
export async function iniciarCorrida(dimensao: 4 | 8 = 4): Promise<Tentativa> {
  const { data } = await apiClient.post<Tentativa>('/v1/runs/tentativas/iniciar/', { dimensao });
  return data;
}

export interface TrajetoriaPonto {
  x: number;
  y: number;
  battery?: number;
}

/** Trajetória histórica de uma tentativa (lista ordenada de posições com bateria). */
export async function fetchTrajetoria(runId: string): Promise<TrajetoriaPonto[]> {
  const { data } = await apiClient.get<TrajetoriaPonto[]>(`/v1/runs/tentativas/${runId}/trajetoria/`);
  return data;
}
