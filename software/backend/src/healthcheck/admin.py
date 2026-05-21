from django.contrib import admin

from healthcheck.models import ServiceCheck


@admin.register(ServiceCheck)
class ServiceCheckAdmin(admin.ModelAdmin):
    list_display = ("name", "url", "is_active", "last_status", "last_checked_at")
    list_filter = ("is_active", "last_status")
    search_fields = ("name", "url")
