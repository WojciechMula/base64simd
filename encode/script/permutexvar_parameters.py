# array generator for _mm512_permutexvar_epi8

def main():
    arr  = generate_array()
    code = render(arr)
    print code


def generate_array():
    shuffle_input_tbl = [0] * 64

    src_index = 0
    for i in xrange(0, 64, 4):
        # 32-bit input: [00000000|ccdddddd|bbbbcccc|aaaaaabb]
        #                            2         1        0
        # output order  [1, 2, 0, 1], i.e.:
        #               [bbbbcccc|ccdddddd|aaaaaabb|bbbbcccc]

        shuffle_input_tbl[i + 0] = src_index + 1
        shuffle_input_tbl[i + 1] = src_index + 0
        shuffle_input_tbl[i + 2] = src_index + 2
        shuffle_input_tbl[i + 3] = src_index + 1
        src_index += 3

    return shuffle_input_tbl


def render(table):
    dwords = []
    for i in xrange(0, 64, 4):
        b0 = table[i + 0]
        b1 = table[i + 1]
        b2 = table[i + 2]
        b3 = table[i + 3]

        dword = (b3 << 24) | (b2 << 16) | (b1 << 8) | b0
        dwords.append(dword)

    return "_mm512_setr_epi32(%s)" % ', '.join('0x%08x' % v for v in dwords)


if __name__ == '__main__':
    main()
