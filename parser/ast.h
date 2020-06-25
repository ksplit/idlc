#include <vector>
#include <gsl/gsl>

namespace idlc {
    /*
        
    */

    enum class item_type {
        rpc,
        projection,
        scope
    };
    
    class projection_field {
    public:
        gsl::czstring<> identifier()
        {
            return m_identifier;
        }

    private:
        gsl::czstring<> m_identifier;
    };

    class item {
    public:
        virtual item_type type() const noexcept = 0;

    private:
        item_type m_type;
    };

    class rpc : public item {
    public:
        item_type type() const noexcept override
        {
            return item_type::rpc;
        }

        gsl::czstring<> identifier()
        {
            return m_identifier;
        }

    private:
        gsl::czstring<> m_identifier;
    };

    class scope : public item {
    public:
        item_type type() const noexcept override
        {
            return item_type::scope;
        }

        gsl::span<item*> items()
        {
            return m_items;
        }

    private:
        std::vector<item*> m_items;
    };

    class projection : public item {
    public:
        item_type type() const noexcept override
        {
            return item_type::projection;
        }

        gsl::span<projection_field> fields()
        {
            return m_fields;
        }

    private:
        std::vector<projection_field> m_fields;
    };

    class file {
    public:
        gsl::span<item*> items()
        {
            return m_items;
        }

    private:
        std::vector<item*> m_items;
    };
}