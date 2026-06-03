from rest_framework.routers import DefaultRouter

from healthcheck.api.views import ServiceCheckViewSet

router = DefaultRouter()
router.register(r"checks", ServiceCheckViewSet, basename="servicecheck")

urlpatterns = router.urls
