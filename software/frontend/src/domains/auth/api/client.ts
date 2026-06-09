import axios, { type AxiosInstance, type InternalAxiosRequestConfig } from 'axios';
import { ACCESS_TOKEN_KEY, REFRESH_TOKEN_KEY } from '@/lib/constants';
import { API_BASE_URL } from './config';

/**
 * apiClient — singleton axios usado por TODO o app autenticado.
 * NUNCA duplicar essa instância em outro domain.
 */
const apiClient: AxiosInstance = axios.create({
  baseURL: API_BASE_URL,
  headers: { 'Content-Type': 'application/json' },
});

// Singleton de refresh em voo: garante que múltiplos 401s simultâneos
// compartilhem o mesmo refresh em vez de dispararem N refreshes em paralelo.
let refreshPromise: Promise<string> | null = null;

apiClient.interceptors.request.use((config: InternalAxiosRequestConfig) => {
  const token = localStorage.getItem(ACCESS_TOKEN_KEY);
  if (token) {
    config.headers.Authorization = `Bearer ${token}`;
  }
  return config;
});

interface ApiEnvelope<T> {
  success?: boolean;
  data?: T;
  pagination?: unknown;
}

apiClient.interceptors.response.use(
  (response) => {
    const { data } = response;
    if (!data || typeof data !== 'object') return response;
    const envelope = data as ApiEnvelope<unknown>;
    if (envelope.pagination !== null && envelope.pagination !== undefined) {
      return {
        ...response,
        data: { results: envelope.data, pagination: envelope.pagination },
      };
    }
    if ('data' in envelope) {
      return { ...response, data: envelope.data };
    }
    return response;
  },
  async (error) => {
    const originalRequest = error.config as
      | (InternalAxiosRequestConfig & { _retry?: boolean })
      | undefined;

    if (
      error.response?.status === 401 &&
      originalRequest &&
      !originalRequest._retry
    ) {
      originalRequest._retry = true;

      if (!refreshPromise) {
        const refreshToken = localStorage.getItem(REFRESH_TOKEN_KEY);
        if (!refreshToken) {
          clearTokens();
          window.location.href = '/login';
          return Promise.reject(error);
        }
        refreshPromise = axios
          .post(`${API_BASE_URL}/v1/token/refresh/`, { refresh: refreshToken })
          .then((res) => {
            const payload = res.data as { data?: { access?: string }; access?: string };
            const newToken: string | undefined = payload.data?.access ?? payload.access;
            if (!newToken) throw new Error('Refresh response missing access token');
            localStorage.setItem(ACCESS_TOKEN_KEY, newToken);
            return newToken;
          })
          .catch((err) => {
            clearTokens();
            window.location.href = '/login';
            throw err;
          })
          .finally(() => {
            refreshPromise = null;
          });
      }

      try {
        const newToken = await refreshPromise;
        if (originalRequest.headers) {
          originalRequest.headers.Authorization = `Bearer ${newToken}`;
        }
        return apiClient(originalRequest);
      } catch {
        return Promise.reject(error);
      }
    }
    return Promise.reject(error);
  },
);

function clearTokens(): void {
  localStorage.removeItem(ACCESS_TOKEN_KEY);
  localStorage.removeItem(REFRESH_TOKEN_KEY);
}

export { apiClient, clearTokens };
