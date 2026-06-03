import type { ReactNode } from 'react';
import { AppShell, Button, Group, Title } from '@mantine/core';
import { useLogout } from '@/domains/auth';

interface AppShellTemplateProps {
  children: ReactNode;
}

export function AppShellTemplate({ children }: AppShellTemplateProps) {
  const logout = useLogout();

  return (
    <AppShell header={{ height: 56 }} padding="md">
      <AppShell.Header>
        <Group h="100%" px="md" justify="space-between">
          <Title order={4}>Micromouse</Title>
          <Button variant="subtle" onClick={logout}>
            Sair
          </Button>
        </Group>
      </AppShell.Header>
      <AppShell.Main>{children}</AppShell.Main>
    </AppShell>
  );
}
