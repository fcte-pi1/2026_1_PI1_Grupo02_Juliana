import { Button, Center, Stack, Text, Title } from '@mantine/core';
import { useNavigate } from 'react-router-dom';

export function DashboardPage() {
  const navigate = useNavigate();

  return (
    <Center mih="60vh">
      <Stack align="center" gap="md" maw={520}>
        <Title order={1} ta="center">
          Bem-vindo ao Micromouse
        </Title>
        <Text c="dimmed" ta="center">
          Próximo passo: criar seu primeiro domain. Use o template{' '}
          <Text span fw={600}>
            healthcheck
          </Text>{' '}
          como referência de domain consumindo a API.
        </Text>
        <Button variant="light" onClick={() => navigate('/healthcheck')}>
          Ver healthcheck
        </Button>
      </Stack>
    </Center>
  );
}
