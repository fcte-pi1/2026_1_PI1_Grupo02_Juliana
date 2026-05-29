import { useQuery } from '@tanstack/react-query';
import { fetchChecks } from '../api';
import type { ServiceCheck } from '../types';

export const HEALTHCHECK_QUERY_KEY = ['healthcheck', 'checks'] as const;

export function useChecks() {
  return useQuery<ServiceCheck[]>({
    queryKey: HEALTHCHECK_QUERY_KEY,
    queryFn: fetchChecks,
  });
}
