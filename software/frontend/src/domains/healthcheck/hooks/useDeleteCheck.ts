import { useMutation, useQueryClient } from '@tanstack/react-query';
import { notifications } from '@mantine/notifications';
import { deleteCheck } from '../api';
import type { ServiceCheck } from '../types';
import { HEALTHCHECK_QUERY_KEY } from './useChecks';

export function useDeleteCheck() {
  const queryClient = useQueryClient();

  return useMutation<void, Error, ServiceCheck>({
    mutationFn: (check) => deleteCheck(check.id),
    onSuccess: (_data, variables) => {
      queryClient.invalidateQueries({ queryKey: HEALTHCHECK_QUERY_KEY });
      notifications.show({
        color: 'green',
        title: 'Check excluído',
        message: `'${variables.name}' removido.`,
      });
    },
    onError: (_error, variables) => {
      notifications.show({
        color: 'red',
        title: 'Falha ao excluir',
        message: `Não consegui excluir '${variables.name}'.`,
      });
    },
  });
}
