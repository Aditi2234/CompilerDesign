from flask import Flask, render_template, request
import subprocess
import re

app = Flask(__name__)
@app.route("/")
def landing():
    return render_template("landing.html")

@app.route("/compiler", methods=["GET", "POST"])
def index():
    code= ""
    tac_output=""
    python_output=""
    error=""
    warning=""

    if request.method=="POST":
        code=request.form.get("code") or ""
        try:
            # Check for invalid header files
            included_headers=re.findall(r'#\s*include\s*<\s*([^>]+)\s*>', code)
            valid_headers=['stdio.h']
            for header in included_headers:
                h = header.strip()
                if h not in valid_headers:
                    error = f"fatal error: {h}: No such file or directory"
                    return render_template(
                        "index.html",
                        code=code, tac_output=tac_output,
                        python_output=python_output,
                        error=error, warning=warning
                    )
            result = subprocess.run(
                ["./compiler"],
                input=code,
                text=True,
                capture_output=True
            )
            output=result.stdout.strip()
            stderr=result.stderr.strip()

            if "Syntax Error" in output:
                error = output

            elif "Semantic Error" in output:
                error = output

            elif result.returncode != 0:
                error = stderr or output or "Compilation failed."

            else:
                uses_io = re.search(r'\b(printf|scanf)\b', code)
                has_stdio = re.search(r'#\s*include\s*<\s*stdio\.h\s*>', code)
                if uses_io and not has_stdio:
                    error = "Semantic Error: printf/scanf used without #include <stdio.h>"
                else:
                    warning_lines = [l for l in output.split("\n") if l.strip().startswith("Warning:")]
                    warning = "\n".join(warning_lines)

                    clean_output = "\n".join(l for l in output.split("\n") if not l.strip().startswith("Warning:"))

                    if "3AC OUTPUT:" in clean_output:
                        tac_raw=clean_output.split("3AC OUTPUT:")[-1].strip()
                    else:
                        tac_raw=clean_output.strip()

                    python_output=convert_to_python(tac_raw)

                    tac_lines = []
                    for line in tac_raw.split("\n"):
                        s = line.strip()
                        if s.startswith("for_init "):
                            tac_lines.append(s[len("for_init "):])
                            continue
                        if s.startswith("for_update "):
                            tac_lines.append(s[len("for_update "):])
                            continue
                        if s in ("do_while_start", "do_while_end"):
                            continue
                        tac_lines.append(line)
                    tac_output = "\n".join(tac_lines)

        except Exception as e:
            error = str(e)

    return render_template(
        "index.html",
        code=code,
        tac_output=tac_output,
        python_output=python_output,
        error=error,
        warning=warning
    )

def convert_to_python(tac):
    lines = [l.strip() for l in tac.split("\n") if l.strip()]

    out=[]
    temp={}
    types={}
    indent=0
    label_stack=[]
    for_info={}  
    params=[]   
    import re
    def resolve(expr):
        for _ in range(10):  
            new_expr=expr
            for t in sorted(temp.keys(), key=len, reverse=True):
                new_expr=re.sub(rf'\b{t}\b', temp[t], new_expr)
            if new_expr==expr:
                break
            expr = new_expr
        return expr

    i=0
    while i < len(lines):
        line=lines[i]

        if line.startswith("for_init "):
            rest = line[len("for_init "):].strip()
            if "=" in rest:
                var, val = rest.split("=", 1)
                var = var.strip()
                val = val.strip()
                for_info["var"] = var
                for_info["start"] = val
            i += 1
            continue

        if line.startswith("for_update "):
            i += 1
            continue

        if line == "do_while_start":
            start_label = None
            if i + 1 < len(lines) and lines[i+1].endswith(":"):
                start_label = lines[i+1][:-1]
            out.append("    " * indent + "while True:")
            indent += 1
            label_stack.append({"type": "do_while", "start": start_label})
            i += 1
            continue

        if line == "do_while_end":
            if label_stack and label_stack[-1].get("type") == "do_while":
                indent -= 1
                label_stack.pop()
            i += 1
            continue

        if line.startswith("read"):
            var = line.split()[1]
            if types.get(var) == "float":
                out.append("    " * indent + f"{var} = float(input())")
            else:
                out.append("    " * indent + f"{var} = int(input())")
            i += 1
            continue

        if line.startswith("param "):
            param_val = line[len("param "):].strip()
            param_val = resolve(param_val)
            params.append(param_val)
            i += 1
            continue

        if line == "call print":
            if params:
                out.append("    " * indent + "print(" + ", ".join(params) + ")")
                params = []
            i += 1
            continue

        if line.startswith("printf"):
            content = line[line.find("(")+1 : line.rfind(")")].strip()
            parts = []
            current = ""
            in_quotes = False
            for ch in content:
                if ch == '"' :
                    in_quotes = not in_quotes
                    current += ch
                elif ch == ',' and not in_quotes:
                    parts.append(current.strip())
                    current = ""
                else:
                    current += ch
            if current.strip():
                parts.append(current.strip())

            if len(parts) == 1:
                txt = parts[0]
                has_nl = '\\n' in txt
                if has_nl:
                    txt = txt.replace('\\n', '')
                    if txt == '""':
                        out.append("    " * indent + "print()")
                    else:
                        out.append("    " * indent + f"print({txt})")
                else:
                    out.append("    " * indent + f"print({txt})")
                i += 1
                continue

            fmt = parts[0].strip('"')
            has_nl = '\\n' in fmt
            fmt = fmt.replace('\\n', '')
            vars_list = parts[1:]
            segments = re.split(r'%[dfsc]', fmt)

            segments = [seg.strip() for seg in segments]

            result = []
            has_text = False
            for j in range(len(segments)):
                if segments[j]:
                    result.append(f'"{segments[j]}"')
                    has_text = True
                if j < len(vars_list):
                    result.append(vars_list[j])

            out.append("    " * indent + "print(" + ", ".join(result) + ")")
            i += 1
            continue

        if line.endswith(":"):
            label = line[:-1]

            is_if_label = False
            if label_stack:
                top = label_stack[-1]
                if top["type"] == "if" and label in (top.get("true"), top.get("false")):
                    is_if_label = True

            if not is_if_label:
                k = i + 1

                while k < len(lines) and lines[k].startswith("t") and "=" in lines[k]:
                    left, right = lines[k].split("=", 1)
                    temp[left.strip()] = right.strip()
                    k += 1

                if k + 1 < len(lines):
                    l1 = lines[k]
                    l2 = lines[k + 1]

                    if l1.startswith("if ") and l2.startswith("goto"):
                        cond = resolve(l1.split()[1])
                        Ltrue = l1.split()[3]
                        Lfalse = l2.split()[1]

                        if for_info and for_info.get("var"):
                            for_var = for_info["var"]
                            for_start = for_info.get("start", "0")

                            for_end = None
                            for_step = "1"
                            cond_resolved = cond

                            m = re.match(rf'{for_var}\s*<\s*(.+)', cond_resolved)
                            if m:
                                for_end = m.group(1).strip()
                            for scan_j in range(k + 2, len(lines)):
                                if lines[scan_j].startswith("for_update "):
                                    upd = lines[scan_j][len("for_update "):].strip()
                                    um = re.match(rf'{for_var}\s*=\s*{for_var}\s*\+\s*(.+)', upd)
                                    if um:
                                        for_step = um.group(1).strip()
                                    um2 = re.match(rf'{for_var}\s*=\s*{for_var}\s*-\s*(.+)', upd)
                                    if um2:
                                        for_step = "-" + um2.group(1).strip()
                                    break

                            if for_end:
                                if for_step == "1":
                                    range_str = f"range({for_start}, {for_end})"
                                else:
                                    range_str = f"range({for_start}, {for_end}, {for_step})"

                                out.append("    " * indent + f"for {for_var} in {range_str}:")
                                indent += 1

                                label_stack.append({
                                    "type": "for",
                                    "start": label,
                                    "true": Ltrue,
                                    "false": Lfalse
                                })

                                for_info.clear()
                                i = k + 2
                                continue
                            else:
                                for_info.clear()

                        for j in range(k + 2, len(lines)):
                            if lines[j].startswith("goto") and lines[j].split()[1] == label:
                                display_cond = "True" if cond == "1" else cond
                                out.append("    " * indent + f"while {display_cond}:")
                                indent += 1

                                label_stack.append({
                                    "type": "while",
                                    "start": label,
                                    "true": Ltrue,
                                    "false": Lfalse
                                })

                                i = k + 2
                                break
                        else:
                            i += 1

                        continue

        if line.startswith("if ") and "goto" in line:
            parts = line.split()
            cond1 = resolve(parts[1])
            Ltrue = parts[3]

            if label_stack and label_stack[-1].get("type") == "do_while":
                if Ltrue == label_stack[-1].get("start"):
                    next_ln = lines[i + 1] if i + 1 < len(lines) else ""
                    if next_ln.startswith("goto"):
                        out.append("    " * indent + f"if not ({cond1}):")
                        out.append("    " * (indent + 1) + "break")
                        i += 2
                        continue

            next_line = lines[i + 1] if i + 1 < len(lines) else ""

            if next_line.startswith("goto"):
                Lfalse = next_line.split()[1]

                if i + 4 < len(lines):
                    label_line = lines[i + 2]
                    next_line1 = lines[i + 3]
                    next_line2 = lines[i + 4]

                    if label_line == Ltrue + ":" and next_line2.startswith("if "):
                        if "=" in next_line1:
                            l2, r2 = next_line1.split("=", 1)
                            temp[l2.strip()] = r2.strip()

                        cond2 = resolve(next_line2.split()[1])

                        inner_goto = lines[i + 5] if i + 5 < len(lines) else ""
                        inner_false = inner_goto.split()[1] if inner_goto.startswith("goto") else ""

                        if inner_false == Lfalse:
                            out.append("    " * indent + f"if {cond1} and {cond2}:")
                            indent += 1

                            label_stack.append({
                                "type": "if",
                                "true": Ltrue,
                                "false": Lfalse,
                                "end": None
                            })

                            i += 5
                            continue
                        
                if i + 2 < len(lines):
                    or_label_line = lines[i + 2]
                    or_label = or_label_line[:-1] if or_label_line.endswith(":") else ""
                    if or_label == Lfalse:

                        or_k = i + 3
                        while or_k < len(lines) and lines[or_k].startswith("t") and "=" in lines[or_k]:
                            tl, tr = lines[or_k].split("=", 1)
                            temp[tl.strip()] = tr.strip()
                            or_k += 1
                        if or_k < len(lines) and lines[or_k].startswith("if "):
                            cond2 = resolve(lines[or_k].split()[1])
                            out.append("    " * indent + f"if {cond1} or {cond2}:")
                            indent += 1

                            label_stack.append({
                                "type": "if",
                                "true": Ltrue,
                                "false": Lfalse,
                                "end": None
                            })

                            i = or_k + 1
                            continue

                out.append("    " * indent + f"if {cond1}:")
                indent += 1

                label_stack.append({
                    "type": "if",
                    "true": Ltrue,
                    "false": Lfalse,
                    "end": None
                })

                i += 2
                continue

        if line.startswith("goto"):
            if label_stack:
                curr = label_stack[-1]

                if curr.get("type") in ("while", "for") and line.split()[1] == curr["start"]:
                    i += 1
                    continue

                if curr.get("type") == "do_while":
                    i += 1
                    continue

                if curr.get("type") == "if" and curr["end"] is None:
                    curr["end"] = line.split()[1]

            i += 1
            continue

        if line.endswith(":"):
            label = line[:-1]

            if label_stack:
                curr = label_stack[-1]

                if curr["type"] in ("while", "for"):
                    if label == curr["true"]:
                        i += 1
                        continue
                    if label == curr["false"]:
                        indent -= 1
                        label_stack.pop()
                        i += 1
                        continue

                if curr["type"] == "do_while":
                    if label == curr.get("start"):
                        i += 1
                        continue

                if curr["type"] == "if":
                    if label == curr["true"]:
                        i += 1
                        continue
                    if label == curr["false"]:

                        if curr["end"] is not None and curr.get("end") != "__else_done__":

                            k = i + 1
                            while k < len(lines) and lines[k].startswith("t") and "=" in lines[k]:
                                left, right = lines[k].split("=", 1)
                                temp[left.strip()] = right.strip()
                                k += 1

                            if (k + 1 < len(lines) and
                                lines[k].startswith("if ") and "goto" in lines[k] and
                                lines[k+1].startswith("goto")):

                                elif_parts = lines[k].split()
                                elif_cond = resolve(elif_parts[1])
                                elif_Ltrue = elif_parts[3]
                                elif_Lfalse = lines[k+1].split()[1]

                                indent -= 1
                                out.append("    " * indent + f"elif {elif_cond}:")
                                indent += 1

                                label_stack.pop()
                                label_stack.append({
                                    "type": "if",
                                    "true": elif_Ltrue,
                                    "false": elif_Lfalse,
                                    "end": curr["end"]
                                })

                                i = k + 2
                                continue
                            else:

                                indent -= 1
                                out.append("    " * indent + "else:")
                                indent += 1

                                curr["false"] = curr["end"]
                                curr["end"] = "__else_done__"

                                i += 1
                                continue
                        else:
                            indent -= 1
                            label_stack.pop()
                            i += 1
                            continue

            i += 1
            continue

        if line.startswith("return"):
            val = resolve(line.split("return", 1)[1].strip())
            if val != "0":
                out.append("    " * indent + f"return {val}")
            i += 1
            continue

        if "=" in line:
            left, right=line.split("=", 1)
            left, right=left.strip(), right.strip()

            types[left]="float" if "." in right else "int"

            right=right.replace("&&", "and").replace("||", "or")
            right=re.sub(r'!\s*([a-zA-Z0-9_]+)', r'not \1', right)

            if left.startswith("t"):
                temp[left] = right
            else:
                out.append("    " * indent + left + " = " + resolve(right))

            i+=1
            continue
        i+=1

    if not out:
        return ""

    indented = ["    " + l for l in out]
    return (
        "def main():\n"
        + "\n".join(indented)
        + '\n\nif __name__ == "__main__":\n    main()'
    )

if __name__ == "__main__":
    app.run(debug=True,port=5107)
