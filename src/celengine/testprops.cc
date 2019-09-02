#include "property.h"
#include <iostream>
#include <memory>

using namespace celestia::engine;
using namespace std;

class myconfig : IConfigUpdater
{
 public:
    myconfig(const std::shared_ptr<Config> &cfg) : IConfigUpdater(cfg) {}
    void read();
};

void myconfig::read()
{
    beginUpdate();
	auto *v1 = new Value(10.18);
    set("Distance", v1);
	auto *v2 = new Value(std::string("foobar"));
    set("Name", v2);
	auto *v3 = new Value(true);
    set("Visible", v3);
    endUpdate();
}

class Foo
{
    Property<double>    m_p1;
    NumericProperty     m_distance;
    StringProperty      m_name, m_type;
    BoolProperty        m_visible;
    shared_ptr<Config>  m_cfg;

 public:
    Foo(const shared_ptr<Config> &cfg):
        m_cfg       (cfg),
        m_p1        (Property<double>(cfg, "P1", 10.5)),
        m_distance  (NumericProperty(cfg, "distance", 55.1)),
        m_name      (StringProperty(cfg, "name", "baz")),
        m_type      (StringProperty(cfg, "type", "baz")),
        m_visible   (BoolProperty(cfg, "visible", false))
    {
    }
    double p1()
    {
        return m_p1.get();
    }
    double distance()
    {
        return m_distance();
    }
    string name()
    {
        return m_name();
    }
    string type()
    {
        return m_type();
    }
    bool visible()
    {
        return m_visible();
    }
};

int main()
{
    auto c = make_shared<Config>();
    myconfig my(c);
    my.read();
    c->dump();
    Foo f(c);
    cout << f.p1() << ' ' << f.distance() << ' ' << f.name() << ' ' << f.type() << ' ' << f.visible() << '\n';
    auto *v = c->find("Name");
    if (v->getType() == Value::StringType)
        cout << v->getString() << '\n';
    else
        cout << "dunno\n";
    c->dump();
}
