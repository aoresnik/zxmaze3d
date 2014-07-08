package zxmaze3dgenprecalc;

/**
 *
 */
public class HT4x4Generate {

    private static int N_INTENSITIES = 17;
    static byte bayer_4x4[] = new byte[]{
        1, 9, 3, 11,
        13, 5, 15, 7,
        4, 12, 2, 10,
        16, 8, 14, 6
    };
    /**
     * 8x8 bitmaps blocks of each intensity, 8 bytes each
     */
    static byte ht_bits[] = new byte[N_INTENSITIES * 8];

    /**
     * Returns 1 if halftone pixel is black, 0 if white
     */
    static boolean ht_is_filled(int x, int y, int intensity) {
        return bayer_4x4[(x & 3) + (y & 3) * 4] > intensity;
    }

    static void span_init() {
        int i;
        int j;
        int k;
        byte b;
        int pos = 0;
        for (i = 0; i < N_INTENSITIES; i++) {
            for (j = 0; j < 4; j++) {
                b = 0;
                for (k = 0; k < 8; k++) {
                    if (ht_is_filled(j, k, i)) {
                        b = (byte) (b | (1 << k));
                    }
                }
                ht_bits[pos + j] = (byte) b;
                ht_bits[pos + j + 4] = (byte) b;
            }
            pos += 8;
        }
    }

    public static void main(String[] argv) {
        span_init();
        int i;
        int j;
        for (i = 0; i < N_INTENSITIES; i++) {
            for (j = 0; j < 8; j++) {
                System.out.format("0x%02x, ", ht_bits[i * 8 + j]);
            }
            System.out.println();
        }
    }
}
