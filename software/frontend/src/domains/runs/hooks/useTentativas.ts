import { useQuery } from '@tanstack/react-query';
import { fetchTentativas } from '../api';
import type { Tentativa } from '../types';

export const TENTATIVAS_QUERY_KEY = ['runs', 'tentativas'] as const;

export function useTentativas() {
  return useQuery<Tentativa[]>({
    queryKey: TENTATIVAS_QUERY_KEY,
    queryFn: fetchTentativas,
  });
}
