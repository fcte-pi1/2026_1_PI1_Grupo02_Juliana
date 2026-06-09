import { useMutation, useQueryClient } from '@tanstack/react-query';
import { notifications } from '@mantine/notifications';
import { createCheck } from '../api';
import type { CreateServiceCheckPayload, ServiceCheck } from '../types';
import { HEALTHCHECK_QUERY_KEY } from './useChecks';

export function useCreateCheck() {
  const queryClient = useQueryClient();

  return useMutation<ServiceCheck, Error, CreateServiceCheckPayload>({
    mutationFn: createCheck,
    onSuccess: (data) => {
      queryClient.invalidateQueries({ queryKey: HEALTHCHECK_QUERY_KEY });
      notifications.show({
        color: 'green',
        title: 'Check criado',
        message: `'${data.name}' adicionado à lista.`,
      });
    },
    onError: () => {
      notifications.show({
        color: 'red',
        title: 'Falha ao criar',
        message: 'Não consegui criar o check. Verifique os campos e tente de novo.',
      });
    },
  });
}
