import { apiClient } from '@/domains/auth/api/client';
import type { PaginatedResponse } from '@/lib/types';
import type { CreateServiceCheckPayload, ServiceCheck } from '../types';

/**
 * Lista todos os checks. O backend devolve paginated; o response interceptor
 * do apiClient já transforma em `{ results, pagination }`. Aqui só pegamos
 * `results` — paginação cliente-side será problema do dia que tiver volume.
 */
export async function fetchChecks(): Promise<ServiceCheck[]> {
  const { data } = await apiClient.get<PaginatedResponse<ServiceCheck>>(
    '/v1/healthcheck/checks/',
  );
  return data.results;
}

export async function createCheck(
  payload: CreateServiceCheckPayload,
): Promise<ServiceCheck> {
  const { data } = await apiClient.post<ServiceCheck>(
    '/v1/healthcheck/checks/',
    payload,
  );
  return data;
}

export async function runCheck(id: string): Promise<ServiceCheck> {
  const { data } = await apiClient.post<ServiceCheck>(
    `/v1/healthcheck/checks/${id}/run/`,
  );
  return data;
}

export async function deleteCheck(id: string): Promise<void> {
  await apiClient.delete(`/v1/healthcheck/checks/${id}/`);
}
