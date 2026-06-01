import { Navigate, Outlet } from 'react-router-dom';
import { ACCESS_TOKEN_KEY } from '@/lib/constants';

/**
 * Guarda de rota sem chrome: só checa o token e renderiza o `Outlet`.
 * Diferente do `ProtectedRoute`, NÃO envolve no `AppShellTemplate` — para
 * páginas full-screen que têm seu próprio layout (ex.: dashboard de telemetria).
 */
export function RequireAuth() {
  const token = localStorage.getItem(ACCESS_TOKEN_KEY);
  if (!token) {
    return <Navigate to="/login" replace />;
  }
  return <Outlet />;
}
