import { Navigate, Outlet } from 'react-router-dom';
import { ACCESS_TOKEN_KEY } from '@/lib/constants';
import { AppShellTemplate } from '@/components/templates/AppShellTemplate';

export function ProtectedRoute() {
  const token = localStorage.getItem(ACCESS_TOKEN_KEY);

  if (!token) {
    return <Navigate to="/login" replace />;
  }

  return (
    <AppShellTemplate>
      <Outlet />
    </AppShellTemplate>
  );
}
