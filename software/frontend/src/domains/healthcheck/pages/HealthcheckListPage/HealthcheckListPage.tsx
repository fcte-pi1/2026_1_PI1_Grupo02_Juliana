import { useState } from 'react';
import {
  Alert,
  Anchor,
  Button,
  Center,
  Group,
  Loader,
  Stack,
  Table,
  Text,
  Title,
} from '@mantine/core';
import { modals } from '@mantine/modals';
import { formatRelative } from '@/lib/formatters';
import { useChecks } from '../../hooks/useChecks';
import { useRunCheck } from '../../hooks/useRunCheck';
import { useDeleteCheck } from '../../hooks/useDeleteCheck';
import { StatusBadge } from '../../components/StatusBadge/StatusBadge';
import { CheckFormModal } from '../../components/CheckFormModal/CheckFormModal';
import type { ServiceCheck } from '../../types';

export function HealthcheckListPage() {
  const [modalOpen, setModalOpen] = useState(false);
  const checksQuery = useChecks();
  const runMutation = useRunCheck();
  const deleteMutation = useDeleteCheck();

  const handleDelete = (check: ServiceCheck) => {
    modals.openConfirmModal({
      title: 'Excluir check',
      centered: true,
      children: (
        <Text size="sm">
          Confirma excluir <strong>{check.name}</strong>? Essa ação não pode ser
          desfeita.
        </Text>
      ),
      labels: { confirm: 'Excluir', cancel: 'Cancelar' },
      confirmProps: { color: 'red' },
      onConfirm: () => deleteMutation.mutate(check),
    });
  };

  const renderBody = () => {
    if (checksQuery.isLoading) {
      return (
        <Center mih={200}>
          <Loader />
        </Center>
      );
    }

    if (checksQuery.isError) {
      return (
        <Alert color="red" title="Erro ao carregar checks">
          {checksQuery.error.message}
        </Alert>
      );
    }

    const checks = checksQuery.data ?? [];

    if (checks.length === 0) {
      return (
        <Center mih={200}>
          <Stack align="center" gap="xs">
            <Text c="dimmed">
              Nenhum check ainda — clique em &lsquo;Novo check&rsquo; pra começar.
            </Text>
          </Stack>
        </Center>
      );
    }

    return (
      <Table striped highlightOnHover withTableBorder>
        <Table.Thead>
          <Table.Tr>
            <Table.Th>Nome</Table.Th>
            <Table.Th>URL</Table.Th>
            <Table.Th>Status</Table.Th>
            <Table.Th>Última verificação</Table.Th>
            <Table.Th>Ações</Table.Th>
          </Table.Tr>
        </Table.Thead>
        <Table.Tbody>
          {checks.map((check) => {
            const isRunning =
              runMutation.isPending && runMutation.variables?.id === check.id;
            const isDeleting =
              deleteMutation.isPending &&
              deleteMutation.variables?.id === check.id;
            return (
              <Table.Tr key={check.id}>
                <Table.Td>{check.name}</Table.Td>
                <Table.Td>
                  <Anchor href={check.url} target="_blank" rel="noreferrer" size="sm">
                    {check.url}
                  </Anchor>
                </Table.Td>
                <Table.Td>
                  <Group gap="xs">
                    <StatusBadge status={check.last_status} />
                    {check.last_status_code !== null && (
                      <Text size="xs" c="dimmed">
                        HTTP {check.last_status_code}
                      </Text>
                    )}
                  </Group>
                </Table.Td>
                <Table.Td>
                  {check.last_checked_at ? (
                    <Text size="sm">{formatRelative(check.last_checked_at)}</Text>
                  ) : (
                    <Text size="sm" c="dimmed">
                      Nunca
                    </Text>
                  )}
                </Table.Td>
                <Table.Td>
                  <Group gap="xs">
                    <Button
                      size="xs"
                      variant="light"
                      loading={isRunning}
                      onClick={() => runMutation.mutate(check)}
                    >
                      Rodar agora
                    </Button>
                    <Button
                      size="xs"
                      variant="subtle"
                      color="red"
                      loading={isDeleting}
                      onClick={() => handleDelete(check)}
                    >
                      Excluir
                    </Button>
                  </Group>
                </Table.Td>
              </Table.Tr>
            );
          })}
        </Table.Tbody>
      </Table>
    );
  };

  return (
    <Stack gap="lg">
      <Group justify="space-between">
        <Title order={2}>Healthchecks</Title>
        <Button onClick={() => setModalOpen(true)}>Novo check</Button>
      </Group>
      {renderBody()}
      <CheckFormModal opened={modalOpen} onClose={() => setModalOpen(false)} />
    </Stack>
  );
}
