import { Badge } from '@mantine/core';
import { STATUS_COLORS } from '@/lib/constants';
import type { ServiceCheckStatus } from '../../types';

const STATUS_LABELS: Record<ServiceCheckStatus, string> = {
  ok: 'OK',
  fail: 'Falhou',
  unknown: 'Desconhecido',
};

interface StatusBadgeProps {
  status: ServiceCheckStatus;
}

export function StatusBadge({ status }: StatusBadgeProps) {
  return (
    <Badge color={STATUS_COLORS[status] ?? 'gray'} variant="light">
      {STATUS_LABELS[status]}
    </Badge>
  );
}
