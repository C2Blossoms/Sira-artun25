import streamlit as st
import pandas as pd
import psycopg2

# ---------------- Database Connect ----------------
conn = psycopg2.connect(
    dbname="postgres",
    user="postgres",
    password="12345678",
    host="localhost",
    port="5432"
)

# ---------------- Query Data ----------------
# 1) ยอดรวม
summary_query = """
SELECT
    SUM(CASE WHEN purpose='topup' THEN amount ELSE 0 END) AS total_topup,
    SUM(CASE WHEN purpose='payment' THEN amount ELSE 0 END) AS total_payment,
    SUM(CASE WHEN purpose='topup' THEN amount ELSE 0 END) -
    SUM(CASE WHEN purpose='payment' THEN amount ELSE 0 END) AS balance_left
FROM transaction_logs;
"""
summary = pd.read_sql(summary_query, conn).iloc[0]

# 2) ลูกค้า (แก้ชื่อตัวสะกด student_id)
customers_query = """
SELECT c.card_id, s.full_name, c.balance
FROM cards c
JOIN students s ON c.student_id = s.student_id
ORDER BY c.card_id
LIMIT 10;
"""
customers = pd.read_sql(customers_query, conn)

# 3) ร้านค้า
vendors_query = """
SELECT v.vendor_id, v.shop_name, COALESCE(SUM(t.amount),0) AS total_spent
FROM vendors v
LEFT JOIN transaction_logs t ON v.vendor_id = t.vendor_id AND t.purpose='payment'
GROUP BY v.vendor_id, v.shop_name
ORDER BY total_spent DESC
LIMIT 10;
"""
vendors = pd.read_sql(vendors_query, conn)

# ---------------- Streamlit Layout ----------------
st.title("📊 School Cashless Dashboard")

# แถวแรก: Summary
col1, col2, col3 = st.columns(3)
col1.metric("💰 เงินที่ถูกเติมเข้าสู่ระบบ", f"{summary['total_topup']:.2f}")
col2.metric("🛒 เงินที่ลูกค้าใช้จ่าย", f"{summary['total_payment']:.2f}")
col3.metric("💳 ยอดเงินคงเหลือรวม", f"{summary['balance_left']:.2f}")

# แถวสอง: กราฟรายบัตร
st.subheader("📈 กราฟการเติมเงิน/จ่ายเงินตามบัตร")

# เลือกบัตร
card_list_query = "SELECT card_id FROM cards ORDER BY card_id;"
card_list = pd.read_sql(card_list_query, conn)["card_id"].tolist()
selected_card = st.selectbox("เลือกบัตร", card_list)

# Query ข้อมูลบัตรที่เลือก (แก้ให้รองรับ TEXT โดยใช้ %s แทน f-string)
card_query = """
SELECT DATE_TRUNC('month', txn_time)::date AS month,
       SUM(amount) FILTER (WHERE purpose='topup') AS total_topup,
       SUM(amount) FILTER (WHERE purpose='payment') AS total_payment
FROM transaction_logs
WHERE card_id = %s
GROUP BY DATE_TRUNC('month', txn_time)
ORDER BY month;
"""
card_data = pd.read_sql(card_query, conn, params=(selected_card,))

if not card_data.empty:
    st.line_chart(card_data.set_index("month")[["total_topup", "total_payment"]])
else:
    st.info("ยังไม่มีข้อมูลสำหรับบัตรนี้")

# แถวสาม: ตาราง ลูกค้า + ร้านค้า
col1, col2 = st.columns(2)
with col1:
    st.subheader("👤 ลูกค้า")
    st.dataframe(customers)
with col2:
    st.subheader("🏪 ร้านค้า")
    st.dataframe(vendors)
