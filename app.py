import streamlit as st
import subprocess
import pandas as pd
import matplotlib.pyplot as plt
import os


st.set_page_config(page_title="Disk Scheduler", layout="wide")


st.markdown("""
<style>
    /* Import IM Fell DW Pica from Google Fonts */
    @import url('https://fonts.googleapis.com/css2?family=IM+Fell+DW+Pica:ital@0;1&display=swap');

    /* Global Background: Soft Baby Pink */
    .stApp { background-color: #FDF2F4; }
    
    /* Text colors: Dark Sage Green & Font application */
    h1, h2, h3, p, span, label, .stMarkdown { 
        color: #5A6B53 !important; 
        font-family: 'IM Fell DW Pica', serif !important;
    }

    /* Specifically target headings to ensure they use the font */
    h1, h2, h3 {
        font-family: 'IM Fell DW Pica', serif !important;
    }
    
    /* Primary Button: Sage Green */
    .stButton>button {
        background-color: #9CAF88 !important;
        color: white !important;
        border-radius: 12px !important;
        border: none !important;
        padding: 10px 24px !important;
        font-weight: bold !important;
        transition: 0.3s;
        font-family: 'IM Fell DW Pica', serif !important;
        font-size: 18px !important;
    }
    .stButton>button:hover {
        background-color: #7A8E65 !important;
    }
    
    /* Input Fields */
    .stTextInput>div>div>input, .stNumberInput>div>div>input {
        background-color: #FFFFFF !important;
        border: 2px solid #E8B4B8 !important;
        border-radius: 8px !important;
        color: #5A6B53 !important;
        font-family: 'IM Fell DW Pica', serif !important;
        font-size: 16px !important;
    }
    
    /* Selectbox text */
    .stSelectbox div[data-baseweb="select"] > div {
        font-family: 'IM Fell DW Pica', serif !important;
    }
    
    /* Dataframes */
    .stDataFrame { border-radius: 10px; overflow: hidden; border: 1px solid #9CAF88; }
</style>
""", unsafe_allow_html=True)


st.title("Disk Request Scheduling Simulator")
st.markdown("### A head movement dashboard")

col1, col2 = st.columns(2)

with col1:
    disk_size = st.number_input("Disk Size (Max Track)", min_value=1, value=200)
    head = st.number_input("Initial Head Position", min_value=0, value=53)
    direction = st.selectbox("Direction", ["right", "left"])

with col2:
    req_input = st.text_input("Requests (space-separated)", "98 183 37 122 14 124 65 67")
    st.write("") # spacer
    st.write("")
    run_btn = st.button("Run Simulation")

if run_btn:
    
    requests = req_input.strip()
    
    
    stdin_data = f"{disk_size}\n{head}\n{direction}\n{requests}\n"
    
    try:
        
        process = subprocess.run(
            ["./disk_scheduler", "run"], 
            input=stdin_data, 
            text=True, 
            capture_output=True
        )
        
        output = process.stdout
        
        if "Error" in output:
            st.error(output)
        else:
            st.success("Simulation Complete!")
            
            
            st.markdown("### Service Order & Metrics")
            st.code(output, language="text")
            
            
            if os.path.exists("head_paths.png"):
                st.markdown("### Head Movement Paths")
                st.image("head_paths.png", caption="Head Movement Across All Algorithms")
                
            
            if os.path.exists("results.csv"):
                st.markdown("### Experiment Results Data")
                df = pd.read_csv("results.csv")
                st.dataframe(df, width='stretch')
                
    except Exception as e:
        st.error(f"Failed to run the backend. Did you compile it? Error: {e}")