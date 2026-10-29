#include <memory>
// fix round 1 (S1): the chain cap, the real-class refusal, the disagreement refusal, a raw-pointer namesake
namespace ch {
class Valve { public: virtual ~Valve() = default; virtual int turnOn(int x) = 0; };
class ValveImpl : public Valve { public: int turnOn(int x) override { return x; } };
class ValveImpl2 : public Valve { public: int turnOn(int x) override { return x + 1; } };
using V0 = std::shared_ptr<Valve>;
using V1 = V0;
using V2 = V1;
using V3 = V2;
using V4 = V3;
}
class Pipe {
public:
  int d0() { return v0_->turnOn(1); }
  int d3() { return v3_->turnOn(1); }
  int d4() { return v4_->turnOn(1); }
  ch::V0 v0_;
  ch::V3 v3_;
  ch::V4 v4_;
};
namespace e2 {
class Pump { public: virtual ~Pump() = default; virtual int flow(int x) = 0; };
class PumpImpl : public Pump { public: int flow(int x) override { return x; } };
class PumpImpl2 : public Pump { public: int flow(int x) override { return x + 1; } };
using Tank = std::shared_ptr<Pump>;
}
namespace f2 {
class Tank { public: int flow(int x) { return x; } };   // the alias name is ALSO a real class
class Rig { public: int go(int x) { return t_->flow(x); } e2::Tank t_; };
}
namespace rp1 {
class Foo { public: virtual ~Foo() = default; virtual int work(int x) = 0; };
class FooImpl : public Foo { public: int work(int x) override { return x; } };
class FooImpl2 : public Foo { public: int work(int x) override { return x + 1; } };
using Ptr = std::shared_ptr<Foo>;
}
namespace rp2 {
class Bar { public: int work(int x) { return x; } };
typedef Bar* Ptr;   // a raw-pointer typedef shares the smart alias's name
class Holder { public: int go(int x) { return p_->work(x); } Ptr p_; };
}
namespace dis { class Kq { public: virtual ~Kq() = default; virtual int poke(int x) = 0; };
  class KqA : public Kq { public: int poke(int x) override { return x; } };
  class KqB : public Kq { public: int poke(int x) override { return x + 1; } };
  using QPtr = std::shared_ptr<Kq>; }
namespace dis { class Kr { public: int poke(int x) { return x; } }; using QPtr = std::unique_ptr<Kr>; }   // the same scope, another pointee
class DisUser { public: int go(int x) { return q_->poke(x); } dis::QPtr q_; };
