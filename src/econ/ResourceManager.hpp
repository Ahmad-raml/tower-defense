#pragma once
struct Cost { int A=0, B=0, C=0; };

class ResourceManager {
public:
    ResourceManager(int a=0, int b=0, int c=0) : A(a), B(b), C(c) {}

    bool canAfford(const Cost& c) const {
        return A >= c.A && B >= c.B && C >= c.C;
    }
    bool spend(const Cost& c) {
        if (!canAfford(c)) return false;
        A -= c.A; B -= c.B; C -= c.C; return true;
    }
    void add(int a, int b, int c) { A += a; B += b; C += c; }

    int a() const { return A; }
    int b() const { return B; }
    int c() const { return C; }

private:
    int A, B, C;
};
