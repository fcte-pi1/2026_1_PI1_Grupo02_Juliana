export type ServiceCheckStatus = 'unknown' | 'ok' | 'fail';

export interface ServiceCheck {
  id: string;
  name: string;
  url: string;
  expected_status: number;
  interval_seconds: number;
  is_active: boolean;
  last_checked_at: string | null;
  last_status: ServiceCheckStatus;
  last_status_code: number | null;
  last_error: string;
  created_at: string;
  updated_at: string;
}

export interface CreateServiceCheckPayload {
  name: string;
  url: string;
  expected_status?: number;
  interval_seconds?: number;
  is_active?: boolean;
}
