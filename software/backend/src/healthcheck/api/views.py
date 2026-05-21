"""ViewSet do `ServiceCheck`. Views finas — toda lógica em use case/selector."""
from rest_framework import status
from rest_framework.decorators import action
from rest_framework.response import Response
from rest_framework.viewsets import ModelViewSet

from healthcheck.api.serializers import ServiceCheckSerializer
from healthcheck.models import ServiceCheck
from healthcheck.use_cases.run_check import RunCheck


class ServiceCheckViewSet(ModelViewSet):
    queryset = ServiceCheck.objects.all()
    serializer_class = ServiceCheckSerializer
    filterset_fields = ["is_active", "last_status"]
    search_fields = ["name", "url"]
    ordering_fields = ["name", "created_at", "last_checked_at"]

    @action(detail=True, methods=["post"], url_path="run")
    def run(self, request, pk=None):
        check = RunCheck().execute(check_id=str(pk))
        return Response(self.get_serializer(check).data, status=status.HTTP_200_OK)
