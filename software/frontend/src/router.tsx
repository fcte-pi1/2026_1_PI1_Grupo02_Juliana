import { lazy, Suspense, type ReactNode } from 'react';
import { createBrowserRouter } from 'react-router-dom';
import { ProtectedRoute } from '@/domains/auth';
import { FullPageLoader } from '@/components/atoms/FullPageLoader';

const LoginPage = lazy(() =>
  import('@/domains/auth/pages/LoginPage/LoginPage').then((m) => ({ default: m.LoginPage })),
);
const DashboardPage = lazy(() =>
  import('@/pages/DashboardPage').then((m) => ({ default: m.DashboardPage })),
);
const HealthcheckListPage = lazy(() =>
  import('@/domains/healthcheck').then((m) => ({ default: m.HealthcheckListPage })),
);

const withSuspense = (node: ReactNode) => (
  <Suspense fallback={<FullPageLoader />}>{node}</Suspense>
);

export const router = createBrowserRouter([
  {
    path: '/login',
    element: withSuspense(<LoginPage />),
  },
  {
    path: '/',
    element: <ProtectedRoute />,
    children: [
      {
        index: true,
        element: withSuspense(<DashboardPage />),
      },
      {
        path: 'healthcheck',
        element: withSuspense(<HealthcheckListPage />),
      },
    ],
  },
]);
