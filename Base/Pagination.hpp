#pragma once
#include <OmniData/DataTable.hpp>
#include <optional>
#include <string>
#include <vector>

namespace omnisphere::types
{
    /**
     * Universal cursor-based pagination result container.
     * Can hold a raw DataTable or a strongly-typed model vector.
     */
    template <typename T = omnisphere::types::DataTable>
    struct CursorPage
    {
        T dataTable;
        std::optional<int> nextCursor;
        std::optional<int> prevCursor;
        bool hasNextPage = false;
        bool hasPreviousPage = false;
        int totalCount = 0;
    };

    using DataTableCursorPage = CursorPage<omnisphere::types::DataTable>;

    /**
     * Standard pagination input parameters across all repositories and services.
     */
    struct PaginationArgs
    {
        std::optional<int> afterEntry;
        int limit = 20;
    };
} // namespace omnisphere::types
