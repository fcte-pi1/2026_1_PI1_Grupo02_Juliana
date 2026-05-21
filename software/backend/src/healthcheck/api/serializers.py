from rest_framework import serializers

from healthcheck.models import ServiceCheck


class ServiceCheckSerializer(serializers.ModelSerializer):
    class Meta:
        model = ServiceCheck
        fields = "__all__"
        read_only_fields = (
            "id",
            "created_at",
            "updated_at",
            "last_checked_at",
            "last_status",
            "last_status_code",
            "last_error",
        )
