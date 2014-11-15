ip = (1, 0, 0, 0, 0, 0, 0, 0, 1, 1)
win_size = 12
one_over = (0, 1, 1, 0, 0)  # f(t^11)

stream = (1,) * 15 + (0,) + (1,) * 15


def sub(a, b):
    ml = max(len(a), len(b))
    a = (0,) * (ml - len(a)) + a
    b = (0,) * (ml - len(b)) + b
    return tuple((c + b[i]) % 2 for i, c in enumerate(a))


def concat(pre_existing, additional):
    return pre_existing + (additional,)


def f(poly):
    assert(len(poly) <= len(ip))
    if len(poly) == len(ip) and poly[0]:
        print('modding', poly)
        return sub(poly, ip)[-len(ip)+1:]
    else:
        return poly[-len(ip)+1:]


h = (0,) * (len(ip) - 1)
cr = ()
for idx, bit in enumerate(stream):
    if idx - win_size >= 0 and stream[idx - win_size]:
        h = sub(h, one_over)
        print('undoing', h)
    cr = concat(h, bit)
    h = f(cr)
    idx += 1
    print('window', stream[max(0, idx-win_size):idx], 'hash', h)
