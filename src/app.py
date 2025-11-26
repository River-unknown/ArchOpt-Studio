import streamlit as st
import os
import glob
import pandas as pd
import re
from run_sim import run_simulation
from parser import parse_output
from visualizer import generate_comparison_charts

st.set_page_config(page_title="ArchOpt-VP", page_icon="⚡", layout="wide")

def get_project_root():
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def get_projects(root_path):
    projects_dir = os.path.join(root_path, "projects")
    if not os.path.exists(projects_dir): return []
    return [d for d in os.listdir(projects_dir) if os.path.isdir(os.path.join(projects_dir, d))]

def generate_soa_refactoring(aos_code):
    struct_pattern = r"struct\s+(\w+)\s*\{([^}]+)\};"
    match = re.search(struct_pattern, aos_code)
    if not match: return None, "Could not parse struct. Ensure 'struct Name { type member; };' format."
    
    struct_name, members_block = match.group(1), match.group(2)
    members = re.findall(r"\s*(\w+\*?)\s+(\w+);", members_block)
    
    if not members: return None, "No members found in struct."

    soa_name = f"{struct_name}_SoA"
    soa_code = f"// Optimized SoA Layout\ntypedef struct {{\n"
    for dtype, mname in members:
        soa_code += f"    {dtype}* {mname};\n"
    soa_code += f"}} {soa_name};\n\n"
    
    soa_code += f"void allocate_{soa_name}({soa_name}* c, int N) {{\n"
    for dtype, mname in members:
        base_type = dtype.replace('*','').strip()
        soa_code += f"    c->{mname} = ({dtype}*)malloc(N * sizeof({base_type}));\n"
    soa_code += "}"
    
    return struct_name, soa_code

def main():
    root = get_project_root()
    st.sidebar.title("⚡ ArchOpt-VP")
    app_mode = st.sidebar.radio("Navigation", ["🔍 Project Profiler", "💻 Refactoring Playground"])

    if app_mode == "🔍 Project Profiler":
        project_list = get_projects(root)
        if not project_list:
            st.warning("No projects found.")
            return
            
        selected_project = st.sidebar.selectbox("Select Benchmark", project_list)
        project_path = os.path.join(root, "projects", selected_project)

        if st.button("🚀 Run Comparative Analysis", type="primary"):
            c_files = glob.glob(os.path.join(project_path, "*.c"))
            if not c_files:
                st.warning("No .c files found.")
                return

            progress_bar = st.progress(0)
            results = {}

            for i, c_file in enumerate(c_files):
                impl = os.path.basename(c_file).replace('.c', '')
                current_dir = os.getcwd()
                os.chdir(root)
                raw_out = run_simulation(c_file)
                os.chdir(current_dir)
                
                if raw_out:
                    metrics = parse_output(raw_out)
                    if metrics: results[impl] = metrics
                progress_bar.progress((i + 1) / len(c_files))

            if not results:
                st.error("No data collected.")
                return

            generate_comparison_charts(results, selected_project, project_path)
            
            col1, col2 = st.columns([1, 1.5])
            with col1:
                st.subheader("📊 Metrics")
                st.dataframe(pd.DataFrame.from_dict(results, orient='index'))
                
                # --- DIAGNOSTIC ENGINE (CRASH PROOF) ---
                aos = results.get('AoS', {})
                soa = results.get('SoA', {})
                
                if aos and soa:
                    # Use 'or 0' to safely handle None values
                    acyc = aos.get('cpu_cycles_m') or 0
                    scyc = soa.get('cpu_cycles_m') or 0
                    amiss = aos.get('cache_misses') or 0
                    smiss = soa.get('cache_misses') or 0
                    
                    st.subheader("🧠 Diagnostics")
                    
                    # 1. Random Forest Anomaly
                    if acyc > 0 and acyc < scyc and amiss > smiss:
                        st.error("⚠️ **Anomaly Detected (Prefetcher Effect)**")
                        st.write("AoS is faster despite more cache misses.")

                    # 2. KNN Traffic Optimization
                    elif scyc > 0 and scyc < acyc and abs(amiss - smiss) < 10:
                        st.success("✅ **Memory Traffic Optimization**")
                        st.write("SoA is faster with identical cache misses.")

                    # 3. Standard Optimization
                    elif scyc > 0 and scyc < acyc and smiss < amiss:
                        st.success("✅ **Standard Cache Optimization**")
                        st.write("SoA is faster due to reduced cache misses.")
                    
                    else:
                        st.info("No specific anomaly detected.")

            with col2:
                st.subheader("📈 Visualization")
                tab1, tab2, tab3 = st.tabs(["Cycles", "Misses", "Energy"])
                def show(metric, tab):
                    img = os.path.join(project_path, f"{selected_project}_{metric}_comparison.png")
                    with tab:
                        if os.path.exists(img): st.image(img)
                        else: st.warning("No chart")
                
                show('cpu_cycles_m', tab1)
                show('cache_misses', tab2)
                show('energy_1e13_nj', tab3)

    elif app_mode == "💻 Refactoring Playground":
        st.header("🛠️ Auto-Refactor: AoS to SoA")
        c1, c2 = st.columns(2)
        with c1:
            default = "struct Point { float x; float y; int id; };"
            user_code = st.text_area("Input C Struct", value=default, height=300)
            if st.button("✨ Refactor"):
                name, res = generate_soa_refactoring(user_code)
                if name:
                    st.session_state['res'] = res
                    st.session_state['ok'] = True
                else:
                    st.error(res)
                    st.session_state['ok'] = False
        with c2:
            if st.session_state.get('ok'):
                st.success("✅ Optimized Code Generated")
                st.code(st.session_state['res'], language='c')

if __name__ == "__main__":
    main()