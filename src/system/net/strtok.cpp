/*
COPYRIGHT AND PERMISSION NOTICE

Copyright (c) 1996 - 2025, Daniel Stenberg, daniel@haxx.se, and many contributors, see the
THANKS file.

All rights reserved.

Permission to use, copy, modify, and distribute this software for any purpose with or
without fee is hereby granted, provided that the above copyright notice and this
permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of a copyright holder shall not be used in
advertising or otherwise to promote the sale, use or other dealings in this Software
without prior written authorization of the copyright holder.
*/

#include "strtok.h"
#include <cstring>

char *Curl_strtok_r(char *ptr, const char *sep, char **end) {
    if (!ptr)
        /* we got NULL input so then we get our last position instead */
        ptr = *end;

    /* pass all letters that are including in the separator string */
    while (*ptr && strchr(sep, *ptr))
        ++ptr;

    if (*ptr) {
        /* so this is where the next piece of string starts */
        char *start = ptr;

        /* set the end pointer to the first byte after the start */
        *end = start + 1;

        /* scan through the string to find where it ends, it ends on a
           null byte or a character that exists in the separator string */
        while (**end && !strchr(sep, **end))
            ++*end;

        if (**end) {
            /* the end is not a null byte */
            **end = '\0'; /* zero terminate it! */
            ++*end; /* advance the last pointer to beyond the null byte */
        }

        return start; /* return the position where the string starts */
    }

    /* we ended up on a null byte, there are no more strings to find! */
    return NULL;
}
