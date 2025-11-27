import streamlit as st
import os
import glob
import pandas as pd
import re
import matplotlib.pyplot as plt

# --- Backend Imports ---
try:
    from run_sim import run_simulation
    from parser import parse_output
    from visualizer import generate_comparison_charts
except ImportError:
    def run_simulation(f): return "Simulation Output Mock"
    def parse_output(o): return {'cpu_cycles_m': 150.5, 'cache_misses': 200, 'energy_1e13_nj': 1.2}
    def generate_comparison_charts(res, proj, path): return {}

# --- Configuration ---
st.set_page_config(page_title="ArchOpt-VP", page_icon="⚡", layout="wide")

# --- CUSTOM CSS: Refined Sci-Fi Theme ---
def inject_custom_css():
    st.markdown("""
    <style>
        /* Import Fonts */
        @import url('https://fonts.googleapis.com/css2?family=Orbitron:wght@400;600;900&family=Rajdhani:wght@400;600&display=swap');

        /* Global Reset */
        .stApp {
            background-color: #050510;
            color: #c0c0d0;
            font-family: 'Rajdhani', sans-serif;
        }

        /* Typography Scaling */
        h1 {
            font-family: 'Orbitron', sans-serif !important;
            font-size: 2.2rem !important;
            font-weight: 900 !important; /* Thicker font weight */
            color: #00f3ff !important;
            text-transform: uppercase;
            margin-top: 0px !important; /* Remove top spacing */
            margin-bottom: 0.2rem !important;
            text-shadow: 0 0 15px rgba(0, 243, 255, 0.4);
        }
        
        h2, h3 {
            font-family: 'Orbitron', sans-serif !important;
            font-size: 1.3rem !important;
            color: #fff !important;
            margin-top: 1.5rem !important;
            margin-bottom: 0.8rem !important;
            letter-spacing: 1.2px;
            text-transform: uppercase;
        }
        
        /* Hide Default Sidebar */
        [data-testid="stSidebar"] {
            display: none;
        }

        /* Top Bar Logo Styling */
        .app-logo {
            font-family: 'Orbitron', sans-serif;
            font-size: 1.8rem;
            font-weight: 700;
            background: -webkit-linear-gradient(#00f3ff, #0099ff);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            margin-top: 10px;
        }

        /* Metric Cards */
        .metric-card {
            background: rgba(10, 10, 25, 0.6);
            border: 1px solid rgba(0, 243, 255, 0.15);
            border-left: 3px solid #00f3ff;
            border-radius: 4px;
            padding: 12px 15px;
            margin-bottom: 10px;
            position: relative;
            overflow: hidden;
        }
        .metric-label {
            font-family: 'Rajdhani', sans-serif;
            font-size: 0.85rem;
            font-weight: 600;
            color: #8899a6;
            text-transform: uppercase;
            letter-spacing: 1.5px;
            margin-bottom: 4px;
        }
        .metric-value {
            font-family: 'Orbitron', sans-serif;
            font-size: 1.6rem;
            font-weight: 600;
            color: #fff;
        }
        .metric-delta-pos { color: #00ff41; font-family: 'Rajdhani', sans-serif; font-size: 0.9rem; font-weight: bold; }
        .metric-delta-neg { color: #ff0055; font-family: 'Rajdhani', sans-serif; font-size: 0.9rem; font-weight: bold; }

        /* Buttons */
        div.stButton > button {
            background: transparent;
            border: 1px solid #00f3ff;
            color: #00f3ff;
            font-family: 'Orbitron', sans-serif;
            font-size: 0.9rem;
            border-radius: 0px;
            padding: 0.4rem 1rem;
            transition: all 0.2s ease;
        }
        div.stButton > button:hover {
            background: rgba(0, 243, 255, 0.15);
            box-shadow: 0 0 8px rgba(0, 243, 255, 0.4);
            color: #fff;
            border-color: #fff;
        }
        
        /* Custom Tabs for Navigation */
        .stTabs [data-baseweb="tab-list"] {
            gap: 20px; /* Space between nav items */
            background-color: transparent;
            padding: 10px 0;
            border-bottom: 1px solid #1a1a2e;
            justify-content: flex-end; /* Right align tabs */
        }
        .stTabs [data-baseweb="tab"] {
            height: 40px;
            white-space: nowrap;
            background-color: transparent;
            border: none;
            color: #888;
            font-family: 'Orbitron', sans-serif;
            font-size: 1rem;
            padding: 0 10px;
            transition: all 0.2s ease;
        }
        .stTabs [data-baseweb="tab"]:hover {
            color: #00f3ff;
        }
        .stTabs [data-baseweb="tab"][aria-selected="true"] {
            color: #00f3ff;
            border-bottom: 2px solid #00f3ff; /* Underline active tab */
        }
    </style>
    """, unsafe_allow_html=True)

# --- Helpers ---
def get_project_root():
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

def get_projects(root_path):
    projects_dir = os.path.join(root_path, "projects")
    if not os.path.exists(projects_dir): return []
    return [d for d in os.listdir(projects_dir) if os.path.isdir(os.path.join(projects_dir, d))]

def render_metric_card(label, value, delta=None, color="cyan"):
    delta_html = ""
    if delta:
        color_class = "metric-delta-pos" if delta.startswith("-") or "Faster" in delta or "FASTER" in delta else "metric-delta-neg"
        delta_html = f"<div class='{color_class}'>{delta}</div>"
    
    st.markdown(f"""
    <div class="metric-card">
        <div class="metric-label">{label}</div>
        <div class="metric-value">{value}</div>
        {delta_html}
    </div>
    """, unsafe_allow_html=True)

def generate_soa_refactoring(aos_code):
    """
    Parses C struct code (supporting both `struct X { ... };` and `typedef struct X { ... } X;`)
    and generates SoA conversion.
    """
    # --- IMPROVED REGEX ---
    # 1. Try matching 'typedef struct Name { ... } Alias;'
    typedef_pattern = r"typedef\s+struct\s+(\w+)?\s*\{([^}]+)\}\s*(\w+);"
    match = re.search(typedef_pattern, aos_code)
    
    if match:
        # Group 1 might be None if it's 'typedef struct { ... } Alias;'
        struct_tag = match.group(1) 
        members_block = match.group(2)
        struct_alias = match.group(3)
        struct_name = struct_alias # Prefer the alias as the main name
    else:
        # 2. Fallback to simple 'struct Name { ... };'
        simple_pattern = r"struct\s+(\w+)\s*\{([^}]+)\};"
        match_simple = re.search(simple_pattern, aos_code)
        if match_simple:
            struct_name = match_simple.group(1)
            members_block = match_simple.group(2)
        else:
            return None, "Could not parse struct. Ensure valid C syntax (e.g., 'typedef struct Name { ... } Name;' or 'struct Name { ... };')."

    # Parse members: "type name;" or "unsigned int name;"
    # Captures type (including spaces/pointers) and name
    member_pattern = r"\s*([a-zA-Z0-9_ ]+\*?)\s+(\w+);"
    members = re.findall(member_pattern, members_block)
    
    if not members: return None, "No valid members found in struct body."
    
    # Generate Code
    soa_name = f"{struct_name}_SoA"
    soa_code = f"// [Optimized Layout] Struct-of-Arrays (SoA)\n"
    soa_code += f"typedef struct {{\n"
    
    for dtype, mname in members:
        clean_type = dtype.strip()
        soa_code += f"    {clean_type}* {mname};\n"
    
    soa_code += f"}} {soa_name};\n\n"
    
    soa_code += f"// [Helper] Allocator\n"
    soa_code += f"void allocate_{soa_name}({soa_name}* c, int N) {{\n"
    for dtype, mname in members:
        clean_type = dtype.strip()
        base_type = clean_type.replace('*', '') # Remove pointer for sizeof
        soa_code += f"    c->{mname} = ({clean_type}*)malloc(N * sizeof({base_type}));\n"
    soa_code += "}"
    
    return struct_name, soa_code

# --- Main App ---
def main():
    inject_custom_css()
    
    root = get_project_root()
    
    # --- TOP HEADER LAYOUT ---
    col_brand, col_nav = st.columns([1, 3])
    
    with col_brand:
        st.markdown('<div class="app-logo">⚡ ARCHOPT-VP</div>', unsafe_allow_html=True)
        
    with col_nav:
        main_tab1, main_tab2 = st.tabs(["PROFILER CORE", "CODE REFACTORER"])

    # ==========================
    # 1. PROFILER CORE
    # ==========================
    with main_tab1:
        project_list = get_projects(root)
        
        c1, c2 = st.columns([3, 1])
        with c1:
            st.markdown("# SYSTEM PROFILER")
            st.caption("Analyze architectural anomalies in ML kernels.")
        with c2:
            if project_list:
                selected_project = st.selectbox("SELECT TARGET", project_list, label_visibility="collapsed")
            else:
                selected_project = None

        st.markdown("---") 

        if selected_project:
            project_path = os.path.join(root, "projects", selected_project)
            
            if st.button("🚀 INITIALIZE SCAN"):
                c_files = glob.glob(os.path.join(project_path, "*.c"))
                if not c_files:
                    st.error("NO SOURCE FILES DETECTED.")
                else:
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
                        st.error("DATA ACQUISITION FAILED.")
                    else:
                        generate_comparison_charts(results, selected_project, project_path)
                        
                        st.markdown("### ⚡ TELEMETRY")
                        
                        aos = results.get('AoS', {})
                        soa = results.get('SoA', {})
                        
                        if aos and soa:
                            acyc = aos.get('cpu_cycles_m', 0)
                            scyc = soa.get('cpu_cycles_m', 0)
                            
                            diff = 0
                            delta_str = "N/A"
                            if acyc > 0 and scyc > 0:
                                diff = acyc - scyc
                                pct = (diff / acyc) * 100
                                if diff > 0:
                                    delta_str = f"SOA IS {pct:.1f}% FASTER"
                                else:
                                    delta_str = f"SOA IS {abs(pct):.1f}% SLOWER"

                            m1, m2, m3 = st.columns(3)
                            with m1: render_metric_card("AOS Cycles", f"{acyc}M")
                            with m2: render_metric_card("SOA Cycles", f"{scyc}M")
                            with m3: render_metric_card("Performance Delta", f"{diff:.1f}M", delta_str)
                            
                            st.markdown("") 
                            amiss = aos.get('cache_misses', 0)
                            smiss = soa.get('cache_misses', 0)
                            
                            if acyc < scyc and amiss > smiss:
                                st.error(f"🛑 ANOMALY DETECTED: PREFETCHER EFFECT [{selected_project}]")
                            elif scyc < acyc and abs(amiss - smiss) < 10:
                                st.success(f"✅ TRAFFIC OPTIMIZATION DETECTED [{selected_project}]")
                            elif scyc < acyc and smiss < amiss:
                                st.success(f"✅ CACHE OPTIMIZATION DETECTED [{selected_project}]")
                            
                        st.markdown("### 📊 VISUALIZATION MATRIX")
                        
                        tab1, tab2, tab3 = st.tabs(["CPU CYCLES", "CACHE MISSES", "ENERGY"])
                        
                        def show_img(metric, tab):
                            p = os.path.join(project_path, f"{selected_project}_{metric}_comparison.png")
                            with tab:
                                if os.path.exists(p):
                                    st.image(p, use_container_width=True)
                                else:
                                    st.info("Chart not available.")

                        show_img('cpu_cycles_m', tab1)
                        show_img('cache_misses', tab2)
                        show_img('energy_1e13_nj', tab3)
                        
                        with st.expander("VIEW RAW DATA LOGS"):
                            st.dataframe(pd.DataFrame.from_dict(results, orient='index'))

    # ==========================
    # 2. REFACTORING CORE
    # ==========================
    with main_tab2:
        st.markdown("")
        st.markdown("# CODE REFACTORER")
        st.caption("Automated translation of legacy structs to DOD-compliant memory layouts.")
        
        st.markdown("---")
        
        c1, c2 = st.columns(2)
        with c1:
            st.markdown("#### INPUT STREAM (AoS)")
            default = "typedef struct Node {\n    int feature_index;\n    double threshold;\n    int predicted_class;\n    unsigned int left_node_addr;\n    unsigned int right_node_addr;\n} Node;"
            user_code = st.text_area("", value=default, height=350, key="code_in")
            
            if st.button("✨ EXECUTE REFACTORING"):
                name, res = generate_soa_refactoring(user_code)
                if name:
                    st.session_state['res'] = res
                    st.session_state['ok'] = True
                else:
                    st.error(res)
                    st.session_state['ok'] = False
        
        with c2:
            st.markdown("#### OUTPUT STREAM (SoA)")
            if st.session_state.get('ok'):
                st.code(st.session_state['res'], language='c')
                st.success("✔ OPERATION SUCCESSFUL")
            else:
                st.info("Awaiting input stream...")

if __name__ == "__main__":
    main()