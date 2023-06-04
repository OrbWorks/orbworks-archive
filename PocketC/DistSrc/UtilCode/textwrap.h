/$textwrap.h
// textwrap(int left, int top, int right, int bottom, string txt)
// ouput a string to the graphics screen within rectangle
// (left,top)-(right,bottom)

/* Example: Draw a box, and fill it with text

main() {
  frame(1, 20, 20, 80, 80, 0);
  textwrap(20, 20, 80, 80, "This is a long line of text that will wrap around.\nThis is a new line");
}
*/

twparagraph(int left, int top, int right, int bottom, string txt);

textwrap(int left, int top, int right, int bottom, string txt) {
    int pos, y, fh=10, nl;
    
    y = top;
    if (right <= left)
        return y;
    if (bottom <= top)
        return y;

    while (txt) {
        nl = 1;
        pos = strstr(txt, "\n", 0);
        if (pos == -1) {
            pos = strlen(txt);
            nl = 0;
        }
        if (pos == 0) {
            // empty line
            y = y + fh + 1;
            txt = substr(txt, 1, strlen(txt)-1);
        } else {
            y = twparagraph(left, y, right, bottom, substr(txt, 0, pos));
            txt = strright(txt, strlen(txt)-pos-nl);
            if (y + fh > bottom)
                break;
        }
    }
    return y;
}

twparagraph(int left, int top, int right, int bottom, string txt) {
    int cch, cw, sl, sp, lsp, w, y, fh = 10;
    string str;
    
    w = right - left;
    y = top;
    
    while (txt) {
        if (y + fh >= bottom)
            return y;
            
        sl = strlen(txt);
        cw = textwidth(txt);
        if (cw > w) {
            // approximately chop the string
            cch = sl * w / cw;
            
            // if it is too short, add some more characters
            while (cch < sl && textwidth(substr(txt, 0, cch)) < w)
                cch++;
            // if it is too long, chop some more
            while (cch > 0 && textwidth(substr(txt, 0, cch)) > w)
                cch--;
            str = substr(txt, 0, cch);
            // find a space
            sp = 0;
            do {
                lsp = sp;
                sp = strstr(str, " ", lsp+1);
            } while (sp != -1);
            // was there a space?
            if (lsp != 0)
                cch = lsp + 1; // include the space
            else if (cch < 1)
                cch = 1;
            str = substr(txt, 0, cch);
            txt = strright(txt, sl-cch);
            text(left, y, str);
            y = y + fh + 1;
        } else {
            text(left, y, txt);
            y = y + fh + 1;
            break;
        }		
    }
    return y;
}
