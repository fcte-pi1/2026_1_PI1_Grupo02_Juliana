export interface Pagination {
  count: number;
  page: number;
  page_size: number;
  total_pages: number;
  next: string | null;
  previous: string | null;
}

export interface PaginatedResponse<T> {
  results: T[];
  pagination: Pagination;
}
