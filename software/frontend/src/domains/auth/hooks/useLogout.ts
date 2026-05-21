import { useCallback } from 'react';
import { useQueryClient } from '@tanstack/react-query';
import { useNavigate } from 'react-router-dom';
import { logout } from '../api';

export function useLogout() {
  const navigate = useNavigate();
  const queryClient = useQueryClient();

  return useCallback(() => {
    logout();
    queryClient.clear();
    navigate('/login', { replace: true });
  }, [navigate, queryClient]);
}
