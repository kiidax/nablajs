/* Nabla JS - A small EMCAScript interpreter with straight-forward implementation.
 * Copyright (C) 2014 Katsuya Iida. All rights reserved.
 */

(function (global) {
    var Object = global.Object;
    var String = global.String;
    var Array = global.Array;
    var Number = global.Number;
    var Math = global.Math;
    var Date = global.Date;
    var Boolean = global.Boolean;
    var RegExp = global.RegExp;
    var JSON = global.JSON = {};

    Object.prototype.__lookupGetter__ = function (sprop) {
        var o = this;
        do {
            var desc = Object.getOwnPropertyDescriptor(o, sprop);
            if (desc) {
                return desc.get;
            }
            o = Object.getPrototypeOf(o);
        } while (o);
        return undefined;
    };

    Object.prototype.__defineGetter__ = function (sprop, fun) {
      if (typeof fun !== 'function') throw 'not a function: ' + fun;
        var desc = Object.getOwnPropertyDescriptor(this, sprop);
        var set = desc && desc.set;
        Object.defineProperty(this, sprop, {
            get: fun, set: set, configurable: true, enumerable: true
        });
        return this;
    };

    Object.prototype.__lookupSetter__ = function (sprop) {
        var o = this;
        do {
            var desc = Object.getOwnPropertyDescriptor(o, sprop);
            if (desc) {
                return desc.set;
            }
            o = Object.getPrototypeOf(o);
        } while (o);
        return undefined;
    };

    Object.prototype.__defineSetter__ = function (sprop, fun) {
        if (typeof fun !== 'function') throw 'not a function: ' + fun;
        var desc = Object.getOwnPropertyDescriptor(this, sprop);
        var get = desc && desc.get;
        Object.defineProperty(this, sprop, {
            get: get, set: fun, configurable: true, enumerable: true
        });
        return this;
    };

    Object.prototype.freeze = function (o) {
        return o;
    };

    String.prototype.indexOf = function (searchString, position) {
        searchString = String(searchString);
        position = position === undefined ? 0 : Number(position);
        var searchLen = searchString.length;
        var end = this.length - searchLen;
        while (position <= end) {
            if (this.substring(position, position + searchLen) === searchString) return position;
            position++;
        }
        return -1;
    };

    String.prototype.charAt = function (index) {
        return this.valueOf()[index];
    };
    
    String.prototype.match = function (regexp) {
        if (typeof regexp !== 'object') {
            regexp = new RegExp(regexp);
        }
        return regexp.exec(this);
    };

    String.prototype.substr = function (start, length) {
        // B.2.3 String.prototype.substr (start, length)
        var s = String(this);
        return s.substring(start, length === undefined ? undefined : start + Number(length));
    };

    String.prototype.slice = function (begin, end) {
        if (end === undefined) {
            return this.substring(begin);
        } else if (end < 0) {
            return this.substring(begin, this.length + end);
        } else {
            return this.substring(begin, end);
        }
    };

    String.prototype.split = function (separator, limit) {
        // 15.5.4.14 String.prototype.split (separator, limit)
        if (this === undefined || this === null) throw new TypeError();
        var S = String(this);
        var A = new Array();
        var lengthA = 0;
        if (limit !== undefined) limit = Number(limit);
        if (limit === 0) return A;
        if (separator === undefined) {
            A[0] = S;
            return A;
        }
        if (typeof separator === 'object' && Object.getPrototypeOf(separator) === RegExp.prototype) {
            var R = RegExp(separator.source);
            if (S.length === 0) {
                var z = R.exec(S);
                if (z) {
                    return A;
                } else {
                    A[0] = S;
                    return A;
                }
            }
            var skipped = '';
            while (S.length !== 0) {
                var z = R.exec(S);
                if (z === null) break;
                var e = z.index + z[0].length;
                if (e === 0) {
                    skipped = S[0];
                    S = S.substring(1);
                } else if (z.index === S.length) {
                    break;
                } else {
                    var T = skipped + S.substring(0, z.index);
                    skipped = '';
                    S = S.substring(e);
                    A.push(T);
                }
            }
            A.push(skipped + S);
            return A;
        } else {
            var R = String(separator);
            if (R.length === 0) {
                for (var i = 0; i < S.length; i++) {
                    A.push(S[i]);
                }
                /*
                Array.forEach.apply(S, [ function (x) {
                    A.push(x);
                } ]);
                */
                return A;
            }
            if (S.length === 0) {
                A[0] = S;
                return A;
            }
            while (S.length !== 0) {
                var p = S.indexOf(R);
                if (p === -1) break;
                var T = S.substring(0, p);
                var e = p + R.length;
                S = S.substring(e);
                A.push(T);
            }
            A.push(S);
            return A;
        }
    };

    String.prototype.replace = function (searchValue, replaceValue) {
        var S = String(this);
        if (typeof searchValue === 'object' && Object.getPrototypeOf(searchValue) === RegExp.prototype) {
            print('String.prototype.replace() is not supported');
            var s = '';
            var R = searchValue;
            if (R.length === 0) {
                return replaceValue + S;
            }
            while (S.length !== 0) {
                var z = R.exec(S);
                if (z === null) {
                    return s + S;
                } else {
                    s += S.substring(0, z.index) + replaceValue;
                    S = S.substring(z.index + z[0].length);
                    if (!R.global) return s + S;
                }
            }
            //throw new Error('String.prototype.replace() is not supported');
            return s;
        } else {
            var s = '';
            var R = String(searchValue);
            var z = S.indexOf(R);
            if (z === -1) {
                return S;
            } else {
                return S.substring(0, z) + replaceValue + S.substring(z + R.length);
            }
        }
    };

    Array.prototype.indexOf = function (s) {
        for (var i = 0; i < this.length; i++) {
            if (this[i] === s) return i;
        }
        return -1;
    };

    Array.prototype.map = function (callbackfn, thisArg) {
        var arr = [];
        Array.prototype.forEach.call(this, function (value, index, obj) {
            arr.push(callbackfn.call(thisArg, value, index, obj));
        });
        return arr;
    };

    Array.prototype.join = function (sep) {
        var ret = '';
        for (var i = 0; i < this.length; i++) {
            if (i != 0) ret += sep;
            ret += this[i];
        }
        return ret;
    };

    Array.prototype.push = function () {
        var n = this.length;
        for (var i = 0; i < arguments.length; i++) {
            this[n++] = arguments[i];
        }
        this.length = n;
        return n;
    };

    Array.prototype.shift = function () {
        if (this.length == 0) return undefined;
        return this.splice(0, 1)[0];
    };

    Array.prototype.slice = function (begin, end) {
        var arr = [];
        if (end === undefined) {
            end = this.length;
        } else if (end < 0) {
            end = this.length + end;
        }
        for (var i = begin; i < end; i++) {
            arr.push(this[i]);
        }
        return arr;
    };

    Array.prototype.toString = function toString() {
        var str = 'k';
        var length = this.length;
        for (var i = 0; i < length; i++) {
            if (i != 0) str = str + ',';
            str = str + this[i];
        }
        return str.substring(1);
    };

    Boolean.prototype.toString = function toString() {
        return String(this.valueOf());
    };

    Number.prototype.toString = function toString(radix) {
        if (radix === undefined) return String(this.valueOf());
        throw new Error('Number.prototype.toString: not supported');
    };

    Math.E = 2.7182818284590452354;
    Math.LN10 = 2.302585092994046;
    Math.LN2 = 0.6931471805599453;
    Math.LOG2E = 1.4426950408889634;
    Math.LOG10E = 0.4342944819032518;
    Math.PI = 3.1415926535897932;
    Math.SQRT1_2 = 0.7071067811865476;
    Math.SQRT2 = 1.4142135623730951;

    Math.min = function min(x, y) { return x > y ? y : x; };
    Math.max = function max(x, y) { return x > y ? x : y; };

    Date.now = function () { return new Date().getTime(); };

    Date.prototype.toUTCString = function () {
        return 'Sun, 12 May 2034 05:06:07 GMT';
    };

    RegExp.prototype.test = function test(string) {
        return this.exec(string) !== null;
    };

    RegExp.prototype.toString = function toString() {
        return '/' + this.source + '/';
    };

    JSON.parse = function parse(x) {
        return eval('(' + x.toString() + ')');
    };
    
    JSON.stringify = function stringify(value, replacer, space) {
        function __stringify__(value, replacer, space, indent) {
            if (Array.isArray(value)) {
                var str = '[';
                var nindent = indent + '    ';
                for (var i = 0; i < value.length; i++) {
                    var elem = value[i];
                    if (elem === undefined) elem = null;
                    str += (i === 0 ? '\n' : ',\n') + nindent + __stringify__(elem, replacer, space, nindent);
                }
                return str + '\n' + indent + ']';
            } else if (value === null) {
                return 'null';
            } else if (typeof value === 'object') {
                var str = null;
                var nindent = indent + '    ';
                for (var i in value) {
                    var elem = value[i];
                    if (elem === undefined) continue;
                    if (str === null) {
                        str = '{\n';
                    } else {
                        str += ',\n';
                    }
                    str += nindent + '\"' + i + '\": ' + __stringify__(elem, replacer, space, nindent);
                }
                return str + '\n' + indent + '}';
            } else if (typeof value === 'string') {
                return '\"' + value.toString() + '\"';
            } else {
                return value.toString();
            }
        }

        return __stringify__(value, replacer, space, '');
    };

    global.isNaN = function (n) {
        return n === NaN;
    };
    
    global.parseInt = function (s) {
        return Math.floor(Number(s));
    };
    
    var cached = {};
    
    global.require = function require(id) {
		var module = cached[id];
		if (typeof module === 'undefined') {
            cached[id] = {
                exports: {},
                module: { id: id, uri: id }
            };
			var source = read(id + '.js');
			source = '(function (require, exports, module) { ' + source + '\n});';
			var func = eval(source);
			
			try {
				func(require, cached[id].exports, cached[id].module);
			} catch (e) {
				delete cached[id];
				throw e;
			}
		}
        return cached[id].exports;
    };
})(this);

// Local Variables:
// js-indent-level: 4
// tab-width: 4
// indent-tabs-mode: nil
// End:
