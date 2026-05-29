import {
  Alert,
  Button,
  Container,
  Paper,
  PasswordInput,
  Stack,
  TextInput,
  Title,
} from '@mantine/core';
import { useForm } from '@mantine/form';
import { useLogin } from '../../hooks/useLogin';

interface LoginFormValues {
  username: string;
  password: string;
}

export function LoginPage() {
  const loginMutation = useLogin();
  const form = useForm<LoginFormValues>({
    initialValues: { username: '', password: '' },
    validate: {
      username: (v) => (v.trim().length === 0 ? 'Informe o usuário' : null),
      password: (v) => (v.length === 0 ? 'Informe a senha' : null),
    },
  });

  const handleSubmit = form.onSubmit((values) => {
    loginMutation.mutate(values);
  });

  return (
    <Container size="xs" py="xl">
      <Paper withBorder shadow="md" p="xl" radius="md" mt="xl">
        <Title order={2} mb="lg" ta="center">
          Micromouse — Entrar
        </Title>
        <form onSubmit={handleSubmit}>
          <Stack>
            <TextInput
              label="Usuário"
              placeholder="seu.usuario"
              required
              autoComplete="username"
              {...form.getInputProps('username')}
            />
            <PasswordInput
              label="Senha"
              placeholder="••••••••"
              required
              autoComplete="current-password"
              {...form.getInputProps('password')}
            />
            {loginMutation.isError && (
              <Alert color="red" title="Falha no login">
                Verifique usuário e senha.
              </Alert>
            )}
            <Button type="submit" loading={loginMutation.isPending} fullWidth>
              Entrar
            </Button>
          </Stack>
        </form>
      </Paper>
    </Container>
  );
}
