#include "property.h"
#include <iostream>

using namespace celestia::engine;
using namespace std;

class Foo
{
    Property<double> m_p1;
    shared_ptr<Config> m_cfg;

 public:
    Foo(shared_ptr<Config> c):
        m_cfg(move(c))
    {
        m_p1 = Property<double>(m_cfg, "P1", 10.5);
    }
    double p1()
    {
        return m_p1.get();
    }
};

int main()
{
    auto c = make_shared<Config>();
    Foo f(c);
    cout << f.p1();
}
