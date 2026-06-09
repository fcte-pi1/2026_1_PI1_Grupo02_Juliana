import { useQuery } from '@tanstack/react-query';
import { fetchSnapshot } from '../api';
import type { RunSnapshot } from '../types';

export function useRunSnapshot(runId: string | null) {
  return useQuery<RunSnapshot>({
    queryKey: ['runs', 'snapshot', runId],
    queryFn: () => fetchSnapshot(runId as string),
    enabled: !!runId,
  });
}
