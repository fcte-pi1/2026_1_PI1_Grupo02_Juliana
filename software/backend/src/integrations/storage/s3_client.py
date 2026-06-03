"""Esqueleto S3/MinIO usando boto3.

Ative com `uv add boto3` no produto que precisar — boto3 NÃO está no
pyproject default do template porque storage é opt-in por produto.

Credenciais default vêm de env (decouple):
- MINIO_ENDPOINT  (ex: http://minio:9000)
- MINIO_ROOT_USER
- MINIO_ROOT_PASSWORD
- MINIO_BUCKET
"""

from decouple import config


class S3Client:
    def __init__(
        self,
        *,
        endpoint: str | None = None,
        access_key: str | None = None,
        secret_key: str | None = None,
        bucket: str | None = None,
    ):
        self._endpoint = endpoint or config("MINIO_ENDPOINT", default=None)
        self._access_key = access_key or config("MINIO_ROOT_USER", default=None)
        self._secret_key = secret_key or config("MINIO_ROOT_PASSWORD", default=None)
        self._bucket = bucket or config("MINIO_BUCKET", default=None)
        self._client = None  # lazy init

    def _get_client(self):
        if self._client is not None:
            return self._client

        # Import lazy — boto3 NÃO está no pyproject default.
        # Ative com `uv add boto3` no produto que consumir esse client.
        import boto3

        if not all([self._endpoint, self._access_key, self._secret_key, self._bucket]):
            raise RuntimeError(
                "S3Client mal configurado: endpoint/access_key/secret_key/bucket "
                "obrigatórios (via env MINIO_* ou kwargs)."
            )

        self._client = boto3.client(
            "s3",
            endpoint_url=self._endpoint,
            aws_access_key_id=self._access_key,
            aws_secret_access_key=self._secret_key,
        )
        return self._client

    def put_object(
        self,
        key: str,
        data: bytes,
        content_type: str = "application/octet-stream",
    ) -> None:
        client = self._get_client()
        client.put_object(
            Bucket=self._bucket,
            Key=key,
            Body=data,
            ContentType=content_type,
        )

    def get_object(self, key: str) -> bytes:
        client = self._get_client()
        response = client.get_object(Bucket=self._bucket, Key=key)
        return response["Body"].read()

    def presigned_url(self, key: str, expires_in: int = 3600) -> str:
        client = self._get_client()
        return client.generate_presigned_url(
            "get_object",
            Params={"Bucket": self._bucket, "Key": key},
            ExpiresIn=expires_in,
        )
