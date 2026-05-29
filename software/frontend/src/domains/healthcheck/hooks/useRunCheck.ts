import { useMutation, useQueryClient } from '@tanstack/react-query';
import { notifications } from '@mantine/notifications';
import { runCheck } from '../api';
import type { ServiceCheck } from '../types';
import { HEALTHCHECK_QUERY_KEY } from './useChecks';

export function useRunCheck() {
  const queryClient = useQueryClient();

  return useMutation<ServiceCheck, Error, ServiceCheck>({
    mutationFn: (check) => runCheck(check.id),
    onSuccess: (_data, variables) => {
      queryClient.invalidateQueries({ queryKey: HEALTHCHECK_QUERY_KEY });
      notifications.show({
        color: 'green',
        title: 'Check executado',
        message: `Check '${variables.name}' executado com sucesso.`,
      });
    },
    onError: (_error, variables) => {
      notifications.show({
        color: 'red',
        title: 'Falha ao executar',
        message: `Não consegui rodar o check '${variables.name}'.`,
      });
    },
  });
}
