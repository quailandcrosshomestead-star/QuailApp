import streamlit as st
import pandas as pd
from datetime import datetime, timedelta
import os

# App Setup
st.set_page_config(page_title="Quail Egg Tracker", page_icon="🥚")
st.title("🥚 Quail Egg Manager")

# File to store data
DATA_FILE = "quail_data.csv"

# Load existing data or create new
if os.path.exists(DATA_FILE):
    df = pd.read_csv(DATA_FILE)
else:
    df = pd.DataFrame(columns=["Date", "Type", "Count", "Notes"])

# Sidebar for Quick Entry
st.sidebar.header("Log New Data")
entry_type = st.sidebar.selectbox("Entry Type", ["Daily Collection", "Incubation Set"])
count = st.sidebar.number_input("Number of Eggs", min_value=1, step=1)
notes = st.sidebar.text_input("Notes (e.g., Pen A, Blue Eggs)")

if st.sidebar.button("Add to Log"):
    new_data = pd.DataFrame([[datetime.now().strftime("%Y-%m-%d"), entry_type, count, notes]], 
                            columns=["Date", "Type", "Count", "Notes"])
    df = pd.concat([df, new_data], ignore_index=True)
    df.to_csv(DATA_FILE, index=False)
    st.sidebar.success("Logged!")

# Dashboard Tabs
tab1, tab2 = st.tabs(["📊 Collection Stats", "🐣 Incubation Timer"])

with tab1:
    st.subheader("Collection History")
    if not df.empty:
        # Filter for only daily collections
        collections = df[df["Type"] == "Daily Collection"]
        st.line_chart(collections.set_index("Date")["Count"])
        st.dataframe(df, use_container_width=True)
    else:
        st.info("No data logged yet.")

with tab2:
    st.subheader("Hatch Calculator")
    set_date = st.date_input("When did you set the eggs?", datetime.now())
    
    # Calculate Quail Timeline
    lockdown = set_date + timedelta(days=15)
    hatch = set_date + timedelta(days=18)
    
    col1, col2 = st.columns(2)
    col1.metric("🔒 Lockdown", lockdown.strftime("%b %d"))
    col2.metric("🐣 Hatch Day", hatch.strftime("%b %d"))
    
    st.warning(f"Stop turning eggs on {lockdown.strftime('%A, %b %d')}!")