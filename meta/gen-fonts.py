#!/usr/bin/python3
import cairo, math


bold = False
size = 48

def init_surf(w, h):
    global surface
    global cr
    surface = cairo.ImageSurface(cairo.FORMAT_A8, w, h)
#    surface = cairo.ImageSurface(cairo.FORMAT_RGB24, w, h)
    cr = cairo.Context(surface)
    cr.select_font_face(
            font,
            cairo.FONT_SLANT_NORMAL,
            bold and cairo.FONT_WEIGHT_BOLD or cairo.FONT_WEIGHT_NORMAL)
    cr.set_font_size(size)


for font, filename in [
    ("robotomono", "font-mono.png"),
    ("robotocondensed", "font-default.png"),
]:

    init_surf(100, 100)


    glyphs = list(map(chr, range(32, 128)))
    glyphs[95] = "•"

    W = 0
    yo = 0
    yp = 0
    for g in glyphs:
        xb, yb, w, h, dx, dy = cr.text_extents(g)
        W = max(W, int(dx) + 1)
        yo = max(yo, h + yb)
        yp = min(yp, yb)
    H = int(yo - yp)

    init_surf(W * 16, H * 8)
    widths = []
    for i, g in enumerate(glyphs):
        xb, yb, w, h, dx, dy = cr.text_extents(g)
        x = i % 16 * W
        y = (2 + i // 16) * H
        if xb < 0:
            x -= xb
            w -= xb
            dx -= xb
            xb = 0

        widths.append(dx)

    #    # debug rectangles
    #    cr.set_source_rgb(0.3, 0, 0)
    #    cr.rectangle(
    #        x + xb,
    #        y + yb + H - yo,
    #        w, h)
    #    cr.fill()
    #    cr.set_source_rgb(0.3, 0, 0.5)
    #    cr.rectangle(
    #        x,
    #        y + yb + H - yo,
    #        dx, 10)
    #    cr.fill()
    #    cr.set_source_rgb(1, 1, 1)

        cr.move_to(x, y + H - yo)
        cr.show_text(g)


    if "mono" in filename:

        x = 0 + W / 2
        y = H + H / 2
        w = W * 0.4
        h = H * 0.3

        # play
        cr.move_to(x - w, y - h)
        cr.line_to(x + w, y)
        cr.line_to(x - w, y + h)
        cr.fill()
        x += W

        # stop
        cr.move_to(x - w, y - h)
        cr.line_to(x + w, y - h)
        cr.line_to(x + w, y + h)
        cr.line_to(x - w, y + h)
        cr.fill()
        x += W

        # pause
        ww = w / 3
        for xx in (x - 2 * ww, x + 2 * ww):
            cr.move_to(xx - ww, y - h)
            cr.line_to(xx + ww, y - h)
            cr.line_to(xx + ww, y + h)
            cr.line_to(xx - ww, y + h)
            cr.fill()
        x += W

        cr.set_line_cap(cairo.LINE_CAP_ROUND)

        # loop
        cr.set_line_width(5)
        r = min(w, h)
        a = -0.2
        a2 = math.pi * 1.5
        si = math.sin(a) * r
        co = math.cos(a) * r
        cr.move_to(x + co, y + si)
        cr.arc(x, y, r, a, a2)
        cr.stroke()
        si = math.sin(a2) * r
        co = math.cos(a2) * r
        cr.save()
        cr.translate(x + co, y + si)
        cr.rotate(a2)
        r *= 0.7
        cr.move_to(-r, -r * 0.2)
        cr.line_to(0, r * 1.2)
        cr.line_to(r, -r * 0.2)
        cr.fill()
        cr.restore()
        cr.stroke()
        x += W

        # noise
        cr.set_line_width(3)
        w = W * 0.45
        h = H * 0.35
        n = [
            0.068653,
            0.113797,
            0.409866,
            -0.18389,
            0.120536,
            0.079325,
            -0.43385,
            0.249644,
            0.033439,
            0.464872,
            0.154274,
        ]
        cr.move_to(x - w, y)
        for f, s in zip(range(-5, 6), n):
            cr.line_to(x + w * f / 6, y + h * s)
        cr.line_to(x + w, y)
        cr.stroke()
        x += W

        h = H * 0.25

        # pulse
        cr.line_to(x - w, y + h)
        cr.line_to(x - w, y - h)
        cr.line_to(x, y - h)
        cr.line_to(x, y + h)
        cr.line_to(x + w, y + h)
        cr.stroke()
        x += W

        # saw
        cr.move_to(x - w, y + h)
        cr.line_to(x + w, y - h)
        cr.line_to(x + w, y + h)
        cr.stroke()
        x += W

        # tri
        cr.move_to(x - w, y + h)
        cr.line_to(x, y - h)
        cr.line_to(x + w, y + h)
        cr.stroke()
        x += W

        # low
        cr.move_to(x - w, y - h)
        cr.line_to(x - w * 0.7, y - h)
        cr.curve_to(
            x + w * 0.3, y - h,
            x + w * 0.3, y,
            x + w * 0.3, y + h)
        cr.line_to(x + w, y + h)
        cr.stroke()
        x += W

        # band
        cr.move_to(x - w, y + h)
        cr.line_to(x - w * 0.7, y + h)
        cr.curve_to(
            x - w * 0.7, y - h,
            x - w * 0.2, y - h,
            x, y - h)
        cr.curve_to(
            x + w * 0.2, y - h,
            x + w * 0.7, y - h,
            x + w * 0.7, y + h)
        cr.line_to(x + w, y + h)
        cr.stroke()
        x += W

        # high
        cr.move_to(x + w, y - h)
        cr.line_to(x + w * 0.7, y - h)
        cr.curve_to(
            x - w * 0.3, y - h,
            x - w * 0.3, y,
            x - w * 0.3, y + h)
        cr.line_to(x - w, y + h)
        cr.stroke()
        x += W


        # arrow left
        w = W / 4
        h = H / 4
        cr.translate(x - w / 3, y)
        cr.move_to(w, -h)
        cr.line_to(-w, 0)
        cr.line_to(w, h)
        cr.fill()
        cr.identity_matrix()
        x += W
        # arrow right
        cr.translate(x + w / 3, y)
        cr.move_to(-w, -h)
        cr.line_to(w, 0)
        cr.line_to(-w, h)
        cr.fill()
        cr.identity_matrix()
        x += W

        # copy
        cr.set_line_cap(cairo.LINE_CAP_BUTT)
        w = W * 0.225
        h = W * 0.3
        def path(ps):
            for i, p in enumerate(ps):
                if i == 0: cr.move_to(p // 16 * w, p % 16 * h)
                else: cr.line_to(p // 16 * w, p % 16 * h)
        cr.translate(x - w * 2, y - h * 2)
        path([ 0x31, 0x30, 0x00, 0x03, 0x13 ])
        cr.stroke()
        path([ 0x11, 0x41, 0x44, 0x14])
        cr.close_path()
        cr.stroke()
        cr.identity_matrix()
        x += W
        # paste
        cr.translate(x - w * 2, y - h * 2)
        path([ 0x31, 0x30, 0x00, 0x03, 0x13 ])
        cr.stroke()
        path([ 0x11, 0x41, 0x44, 0x14])
        cr.close_path()
        cr.fill_preserve()
        cr.stroke()
        cr.identity_matrix()
        x += W


    cr.set_line_cap(cairo.LINE_CAP_BUTT)

    # rounded corners
    S = 32
    for r in [0, 8, 16, 24]:
        cr.move_to(0, S)
        cr.arc(r, r, r, -math.pi, -math.pi / 2)
        cr.line_to(S, 0)
        cr.line_to(S, S)
        cr.close_path()
        cr.fill()
        cr.translate(S + 4, 0)

    L = 6
    cr.set_line_width(L)
    cr.translate(L * 0.5, L * 0.5)
    for r in [0, 8, 16, 24]:
        cr.move_to(0, S - L * 0.5)
        cr.arc(r, r, r, -math.pi, -math.pi / 2)
        cr.line_to(S - L * 0.5, 0)
        cr.stroke()
        cr.translate(S + 4, 0)

    cr.identity_matrix()

    surface.write_to_png(filename)

    enum = "TEX_" + filename[:-4].replace("-", "_").upper()
    print("{")
    print("    %s, %d, %d, {" % (enum, W, H))
    while widths:
        print("    " * 2 + " ".join("%d," % x for x in widths[:16]))
        widths = widths[16:]
    print("    }")
    print("},")
