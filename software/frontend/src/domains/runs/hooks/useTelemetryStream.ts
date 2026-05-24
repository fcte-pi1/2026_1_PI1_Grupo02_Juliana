import { useEffect, useState } from 'react';
import { ACCESS_TOKEN_KEY } from '@/lib/constants';
import { API_BASE_URL } from '@/domains/auth/api/config';
import type { RunSnapshot } from '../types';

interface StreamState {
  snapshot: RunSnapshot | null;
  connected: boolean;
}

/**
 * Abre um EventSource (SSE) no endpoint de stream da tentativa e devolve o
 * último snapshot recebido. O backend manda o `maze` completo a cada tick,
 * então basta substituir o estado — sem merge manual de delta no cliente.
 *
 * Auth: EventSource não envia header Authorization, então o JWT vai por query
 * param (validado manualmente na view SSE).
 */
export function useTelemetryStream(runId: string | null): StreamState {
  const [snapshot, setSnapshot] = useState<RunSnapshot | null>(null);
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    if (!runId) return undefined;

    const token = localStorage.getItem(ACCESS_TOKEN_KEY) ?? '';
    const url = `${API_BASE_URL}/v1/runs/tentativas/${runId}/stream/?token=${encodeURIComponent(token)}`;
    const source = new EventSource(url);

    source.onopen = () => setConnected(true);
    source.onmessage = (event) => {
      try {
        setSnapshot(JSON.parse(event.data) as RunSnapshot);
      } catch {
        // keepalive / payload malformado — ignora
      }
    };
    source.onerror = () => setConnected(false);

    return () => source.close();
  }, [runId]);

  return { snapshot, connected };
}
