from rest_framework.pagination import PageNumberPagination


class StandardPagination(PageNumberPagination):
    """Paginação padrão da API: 20 por página, override via `?page_size=N` (max 200)."""

    page_size = 20
    page_size_query_param = "page_size"
    max_page_size = 200
