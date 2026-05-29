import { useMutation } from '@tanstack/react-query';
import { useNavigate } from 'react-router-dom';
import { login } from '../api';
import type { LoginCredentials, TokenResponse } from '../types';

export function useLogin() {
  const navigate = useNavigate();

  return useMutation<TokenResponse, Error, LoginCredentials>({
    mutationFn: login,
    onSuccess: () => {
      navigate('/', { replace: true });
    },
  });
}
