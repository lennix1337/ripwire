int fill_buffer(int *out, int n);
int scale_and_store(int *out, int *aux, int n)
{
    int t = fill_buffer(out, n);
    *aux = t * 3;
    *out = *out + *aux;
    *aux = *aux - 1;
    *out = *out * 2;
    return *out;
}
int store_scale_factor(int *slot, int factor)
{
    int scaled = factor * 10;
    int stored = scaled + 1;
    *slot = stored;
    return stored;
}
int scale_store_limit(int v)
{
    int lim = v > 100 ? 100 : v;
    int lo = lim < 0 ? 0 : lim;
    return lo * 2;
}
int store_and_scale_twice(int v)
{
    return ( v * 3 ) - ( v % 7 );
}
