/*
 * Auto-level: find the digit band of a meter in a raw camera image and work out
 * the rotation that makes it horizontal and upright.
 *
 * Added in a modified version of AI-on-the-Edge Device [SLFork], not by the original authors.
 * Original project: https://github.com/Slider0007/AI-on-the-edge-device
 *
 * detectMeterRotation(imageData) returns null when no band is found, otherwise
 *   { angle, upright, band: {cx, cy, length, width}, scale }
 * angle:   degrees for InitialRotate, in -180..180 (positive = clockwise, as canvas.rotate)
 * upright: "red-dials" or "position" (how up was decided) or "unknown"
 */
var AutoLevel = (function () {
    var MAX_SIDE = 320;

    function downscale(imageData) {
        var scale = Math.min(1, MAX_SIDE / Math.max(imageData.width, imageData.height));
        var w = Math.round(imageData.width * scale), h = Math.round(imageData.height * scale);
        var gray = new Float32Array(w * h), red = new Uint8Array(w * h), src = imageData.data;
        var step = 1 / scale, box = Math.max(1, Math.floor(step));

        for (var y = 0; y < h; y++) {
            for (var x = 0; x < w; x++) {
                var r = 0, g = 0, b = 0, n = 0;
                var sy0 = Math.floor(y * step), sx0 = Math.floor(x * step);
                for (var sy = sy0; sy < Math.min(sy0 + box, imageData.height); sy++) {
                    for (var sx = sx0; sx < Math.min(sx0 + box, imageData.width); sx++) {
                        var p = (sy * imageData.width + sx) * 4;
                        r += src[p]; g += src[p + 1]; b += src[p + 2]; n++;
                    }
                }
                r /= n; g /= n; b /= n;
                gray[y * w + x] = 0.299 * r + 0.587 * g + 0.114 * b;
                red[y * w + x] = (r > 120 && r > 1.7 * g && r > 1.7 * b) ? 1 : 0;
            }
        }
        return { w: w, h: h, gray: gray, red: red, scale: scale };
    }

    function otsu(gray) {
        var hist = new Array(256).fill(0), i, total = gray.length, sum = 0;
        for (i = 0; i < total; i++) hist[Math.min(255, Math.max(0, gray[i] | 0))]++;
        for (i = 0; i < 256; i++) sum += i * hist[i];
        var sumB = 0, wB = 0, best = 0, thr = 80;
        for (i = 0; i < 256; i++) {
            wB += hist[i];
            if (wB === 0 || wB === total) continue;
            sumB += i * hist[i];
            var mB = sumB / wB, mF = (sum - sumB) / (total - wB);
            var between = wB * (total - wB) * (mB - mF) * (mB - mF);
            if (between > best) { best = between; thr = i; }
        }
        return thr;
    }

    /* Connected components of a binary mask; components touching the border
       are the housing around the meter and are dropped. */
    function components(mask, w, h) {
        var seen = new Uint8Array(w * h), result = [], stack = [];
        for (var start = 0; start < w * h; start++) {
            if (!mask[start] || seen[start]) continue;
            var pts = [], touches = false;
            stack.push(start); seen[start] = 1;
            while (stack.length) {
                var p = stack.pop(), x = p % w, y = (p - x) / w;
                pts.push(p);
                if (x === 0 || y === 0 || x === w - 1 || y === h - 1) touches = true;
                if (x > 0 && mask[p - 1] && !seen[p - 1]) { seen[p - 1] = 1; stack.push(p - 1); }
                if (x < w - 1 && mask[p + 1] && !seen[p + 1]) { seen[p + 1] = 1; stack.push(p + 1); }
                if (y > 0 && mask[p - w] && !seen[p - w]) { seen[p - w] = 1; stack.push(p - w); }
                if (y < h - 1 && mask[p + w] && !seen[p + w]) { seen[p + w] = 1; stack.push(p + w); }
            }
            if (!touches) result.push(pts);
        }
        return result;
    }

    /* Tight box around the points at a given angle (degrees). */
    function boxAt(pts, w, deg) {
        var c = Math.cos(deg * Math.PI / 180), s = Math.sin(deg * Math.PI / 180);
        var minU = 1e9, maxU = -1e9, minV = 1e9, maxV = -1e9;
        for (var i = 0; i < pts.length; i++) {
            var x = pts[i] % w, y = (pts[i] - x) / w;
            var u = x * c + y * s, v = -x * s + y * c;
            if (u < minU) minU = u; if (u > maxU) maxU = u;
            if (v < minV) minV = v; if (v > maxV) maxV = v;
        }
        return { minU: minU, maxU: maxU + 1, minV: minV, maxV: maxV + 1, c: c, s: s };
    }

    function minAreaBox(pts, w) {
        var best = null;
        for (var deg = 0; deg < 180; deg += 2) {
            var b = boxAt(pts, w, deg), L = b.maxU - b.minU, W = b.maxV - b.minV;
            if (L < W) continue;                        // keep u along the long side
            if (!best || L * W < best.area) best = { deg: deg, box: b, area: L * W };
        }
        return best;
    }

    /* Sharpen the angle: the band edges are straight, so the row profile of the
       mask across the band is sharpest at the true angle. */
    function refineAngle(pts, w, deg) {
        var bestDeg = deg, bestScore = -1;
        for (var d = deg - 3; d <= deg + 3.001; d += 0.1) {
            var c = Math.cos(d * Math.PI / 180), s = Math.sin(d * Math.PI / 180), rows = {};
            for (var i = 0; i < pts.length; i++) {
                var x = pts[i] % w, y = (pts[i] - x) / w, v = Math.round((-x * s + y * c) * 2);
                rows[v] = (rows[v] || 0) + 1;
            }
            var score = 0;
            for (var k in rows) score += rows[k] * rows[k];
            if (score > bestScore) { bestScore = score; bestDeg = d; }
        }
        return bestDeg;
    }

    /* Grow the white digits away: a band with bright digits is otherwise full of holes. */
    function closeMask(mask, w, h, r) {
        function pass(input, keepIfAny) {
            var out = new Uint8Array(w * h);
            for (var y = 0; y < h; y++) {
                for (var x = 0; x < w; x++) {
                    var any = false, all = true;
                    for (var dy = -r; dy <= r; dy++) {
                        for (var dx = -r; dx <= r; dx++) {
                            var yy = y + dy, xx = x + dx;
                            var on = (yy >= 0 && yy < h && xx >= 0 && xx < w) ? input[yy * w + xx] : 0;
                            if (on) any = true; else all = false;
                        }
                    }
                    out[y * w + x] = (keepIfAny ? any : all) ? 1 : 0;
                }
            }
            return out;
        }
        return pass(pass(mask, true), false);
    }

    function findBand(img, closeRadius) {
        var w = img.w, h = img.h, thr = otsu(img.gray), mask = new Uint8Array(w * h);
        for (var i = 0; i < w * h; i++) mask[i] = img.gray[i] < thr ? 1 : 0;
        if (closeRadius > 0) mask = closeMask(mask, w, h, closeRadius);

        var comps = components(mask, w, h), best = null, imageArea = w * h;
        for (var c = 0; c < comps.length; c++) {
            var pts = comps[c];
            if (pts.length < imageArea * 0.008 || pts.length > imageArea * 0.25) continue;
            var box = minAreaBox(pts, w);
            if (!box) continue;
            var L = box.box.maxU - box.box.minU, W = box.box.maxV - box.box.minV;
            var aspect = L / W, fill = pts.length / box.area;
            if (aspect < 2.5 || aspect > 9 || fill < 0.6) continue;
            var score = pts.length * fill;
            if (!best || score > best.score) best = { pts: pts, score: score, deg: box.deg };
        }
        return best;
    }

    function detectMeterRotation(imageData) {
        var img = downscale(imageData), found = null;

        // Bands come in many sizes, so try increasing closing radii
        for (var r = 0; r <= 3 && !found; r++) found = findBand(img, r);
        if (!found) return null;

        var deg = refineAngle(found.pts, img.w, found.deg);
        var b = boxAt(found.pts, img.w, deg);
        var cu = (b.minU + b.maxU) / 2, cv = (b.minV + b.maxV) / 2;

        // Axis is known up to 180 degrees. Red dial pointers sit below the digits;
        // v increases towards "below" when the image is rotated by -deg.
        var redV = 0, redN = 0;
        for (var i = 0; i < img.w * img.h; i++) {
            if (!img.red[i]) continue;
            var x = i % img.w, y = (i - x) / img.w;
            redV += -x * b.s + y * b.c - cv; redN++;
        }

        var flip, upright;
        if (redN > img.w * img.h * 0.002 && Math.abs(redV / redN) > (b.maxV - b.minV) * 0.5) {
            flip = redV < 0; upright = "red-dials";
        }
        else {
            // No dials: the band normally sits in the upper half of the meter face
            var centreV = -img.w / 2 * b.s + img.h / 2 * b.c;
            if (Math.abs(centreV - cv) > (b.maxV - b.minV) * 0.3) {
                flip = centreV < cv; upright = "position";
            }
            else {
                flip = false; upright = "unknown";
            }
        }

        var angle = -deg + (flip ? 180 : 0);
        while (angle > 180) angle -= 360;
        while (angle <= -180) angle += 360;

        return {
            angle: Math.round(angle * 10) / 10,
            upright: upright,
            axis: deg,
            band: {
                cx: (cu * b.c - cv * b.s) / img.scale,
                cy: (cu * b.s + cv * b.c) / img.scale,
                length: (b.maxU - b.minU) / img.scale,
                width: (b.maxV - b.minV) / img.scale
            },
            scale: img.scale
        };
    }

    /* How readable is the band? Spread between digits and band background, minus
       burnt-out pixels in and around the band. Higher is better, for comparing
       shots of the same scene. */
    function scoreReadability(imageData, band, axis) {
        var c = Math.cos(axis * Math.PI / 180), s = Math.sin(axis * Math.PI / 180);
        var values = [], clipped = 0, total = 0, d = imageData.data;

        for (var v = -band.width; v <= band.width; v++) {
            for (var u = -band.length / 2; u <= band.length / 2; u++) {
                var x = Math.round(band.cx + u * c - v * s), y = Math.round(band.cy + u * s + v * c);
                if (x < 0 || y < 0 || x >= imageData.width || y >= imageData.height) continue;
                var p = (y * imageData.width + x) * 4;
                var g = 0.299 * d[p] + 0.587 * d[p + 1] + 0.114 * d[p + 2];
                total++;
                if (g >= 250) clipped++;
                if (Math.abs(v) <= band.width * 0.4) values.push(g);
            }
        }
        if (!values.length) return -1;

        values.sort(function (a, b) { return a - b; });
        var dark = values[Math.floor(values.length * 0.2)], bright = values[Math.floor(values.length * 0.98)];
        return (bright - dark) / 255 - clipped / total;
    }

    return { detectMeterRotation: detectMeterRotation, scoreReadability: scoreReadability };
})();
