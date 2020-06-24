#include <vector>
#include <gsl/gsl>

namespace idlc {
    enum class item_type {
        rpc,
        proj
    };
    
    class item {
    public:
        item_type type()
        {
            return m_type;
        }

    private:
        item_type m_type;
    };

    class file {
    public:


    private:
        std::vector<item> items;
    };
}