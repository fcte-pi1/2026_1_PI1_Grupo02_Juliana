import {
  Button,
  Group,
  Modal,
  NumberInput,
  Stack,
  Switch,
  TextInput,
} from '@mantine/core';
import { useForm } from '@mantine/form';
import { useEffect } from 'react';
import { useCreateCheck } from '../../hooks/useCreateCheck';
import type { CreateServiceCheckPayload } from '../../types';

interface CheckFormValues {
  name: string;
  url: string;
  expected_status: number;
  interval_seconds: number;
  is_active: boolean;
}

const INITIAL_VALUES: CheckFormValues = {
  name: '',
  url: '',
  expected_status: 200,
  interval_seconds: 300,
  is_active: true,
};

interface CheckFormModalProps {
  opened: boolean;
  onClose: () => void;
}

export function CheckFormModal({ opened, onClose }: CheckFormModalProps) {
  const createMutation = useCreateCheck();
  const form = useForm<CheckFormValues>({
    initialValues: INITIAL_VALUES,
    validate: {
      name: (v) => (v.trim().length === 0 ? 'Informe um nome' : null),
      url: (v) => {
        if (v.trim().length === 0) return 'Informe a URL';
        try {
          new URL(v);
          return null;
        } catch {
          return 'URL inválida';
        }
      },
      expected_status: (v) =>
        v < 100 || v > 599 ? 'Status HTTP entre 100 e 599' : null,
      interval_seconds: (v) =>
        v < 10 ? 'Intervalo mínimo de 10 segundos' : null,
    },
  });

  // Reset ao reabrir o modal — evita carregar valores da última criação.
  useEffect(() => {
    if (opened) {
      form.setValues(INITIAL_VALUES);
      form.resetDirty(INITIAL_VALUES);
      form.resetTouched();
    }
    // form é estável entre renders no Mantine; não precisa entrar nas deps.
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [opened]);

  const handleSubmit = form.onSubmit((values) => {
    const payload: CreateServiceCheckPayload = {
      name: values.name.trim(),
      url: values.url.trim(),
      expected_status: values.expected_status,
      interval_seconds: values.interval_seconds,
      is_active: values.is_active,
    };
    createMutation.mutate(payload, {
      onSuccess: () => {
        onClose();
      },
    });
  });

  return (
    <Modal opened={opened} onClose={onClose} title="Novo healthcheck" centered>
      <form onSubmit={handleSubmit}>
        <Stack>
          <TextInput
            label="Nome"
            placeholder="API de produção"
            required
            {...form.getInputProps('name')}
          />
          <TextInput
            label="URL"
            placeholder="https://api.exemplo.com/health"
            required
            {...form.getInputProps('url')}
          />
          <NumberInput
            label="Status HTTP esperado"
            min={100}
            max={599}
            {...form.getInputProps('expected_status')}
          />
          <NumberInput
            label="Intervalo (segundos)"
            min={10}
            {...form.getInputProps('interval_seconds')}
          />
          <Switch
            label="Ativo"
            {...form.getInputProps('is_active', { type: 'checkbox' })}
          />
          <Group justify="flex-end">
            <Button variant="subtle" onClick={onClose} type="button">
              Cancelar
            </Button>
            <Button type="submit" loading={createMutation.isPending}>
              Criar
            </Button>
          </Group>
        </Stack>
      </form>
    </Modal>
  );
}
