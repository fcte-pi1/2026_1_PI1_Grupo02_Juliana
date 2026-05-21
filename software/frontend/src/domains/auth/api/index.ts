import axios from 'axios';
import { ACCESS_TOKEN_KEY, REFRESH_TOKEN_KEY } from '@/lib/constants';
import { apiClient, clearTokens } from './client';
import { API_BASE_URL } from './config';
import type { AuthUser, LoginCredentials, TokenResponse } from '../types';

/**
 * Login NÃO usa apiClient. Motivo: o interceptor de response do apiClient
 * trata 401 como "preciso fazer refresh", o que num login causaria loop —
 * 401 no login significa "credencial errada", não "token expirado".
 * Por isso disparamos axios cru direto no endpoint público.
 */
export async function login(credentials: LoginCredentials): Promise<TokenResponse> {
  const response = await axios.post<{ data?: TokenResponse } & Partial<TokenResponse>>(
    `${API_BASE_URL}/v1/token/`,
    credentials,
    { headers: { 'Content-Type': 'application/json' } },
  );

  // Backend pode embrulhar em envelope `{ data: { access, refresh } }` ou devolver flat.
  const payload = response.data;
  const tokens: TokenResponse = {
    access: payload.data?.access ?? payload.access ?? '',
    refresh: payload.data?.refresh ?? payload.refresh ?? '',
  };

  if (!tokens.access || !tokens.refresh) {
    throw new Error('Login response missing tokens');
  }

  localStorage.setItem(ACCESS_TOKEN_KEY, tokens.access);
  localStorage.setItem(REFRESH_TOKEN_KEY, tokens.refresh);
  return tokens;
}

export function logout(): void {
  clearTokens();
}

export async function getMe(): Promise<AuthUser> {
  const { data } = await apiClient.get<AuthUser>('/v1/me/');
  return data;
}
