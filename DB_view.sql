--
-- PostgreSQL database cluster dump
--

-- Started on 2025-09-28 21:53:00

SET default_transaction_read_only = off;

SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;

--
-- Roles
--

CREATE ROLE postgres;
ALTER ROLE postgres WITH SUPERUSER INHERIT CREATEROLE CREATEDB LOGIN REPLICATION BYPASSRLS PASSWORD 'SCRAM-SHA-256$4096:UnJOwKzhjnewGa3KLilscA==$xRJjfc6yAQCVd0Mg3AAtM3sJF6dHEXKZxnhx/vHi568=:4htzgMch4PBk4aWSRKNGL9zXgQ1dRvseboKgk8n/E0o=';

--
-- User Configurations
--








--
-- Databases
--

--
-- Database "template1" dump
--

\connect template1

--
-- PostgreSQL database dump
--

-- Dumped from database version 17.5
-- Dumped by pg_dump version 17.5

-- Started on 2025-09-28 21:53:00

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

-- Completed on 2025-09-28 21:53:00

--
-- PostgreSQL database dump complete
--

--
-- Database "postgres" dump
--

\connect postgres

--
-- PostgreSQL database dump
--

-- Dumped from database version 17.5
-- Dumped by pg_dump version 17.5

-- Started on 2025-09-28 21:53:00

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET transaction_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- TOC entry 222 (class 1259 OID 16482)
-- Name: cards; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.cards (
    card_id text NOT NULL,
    student_id integer,
    balance numeric(10,2)
);


ALTER TABLE public.cards OWNER TO postgres;

--
-- TOC entry 221 (class 1259 OID 16481)
-- Name: cards_card_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.cards_card_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.cards_card_id_seq OWNER TO postgres;

--
-- TOC entry 4931 (class 0 OID 0)
-- Dependencies: 221
-- Name: cards_card_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.cards_card_id_seq OWNED BY public.cards.card_id;


--
-- TOC entry 218 (class 1259 OID 16468)
-- Name: students; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.students (
    student_id integer NOT NULL,
    full_name character varying(100) NOT NULL,
    class character varying(100)
);


ALTER TABLE public.students OWNER TO postgres;

--
-- TOC entry 217 (class 1259 OID 16467)
-- Name: students_stdent_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.students_stdent_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.students_stdent_id_seq OWNER TO postgres;

--
-- TOC entry 4932 (class 0 OID 0)
-- Dependencies: 217
-- Name: students_stdent_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.students_stdent_id_seq OWNED BY public.students.student_id;


--
-- TOC entry 224 (class 1259 OID 16495)
-- Name: transaction_logs; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.transaction_logs (
    txn_id integer NOT NULL,
    card_id text,
    vendor_id integer,
    amount numeric(10,2) NOT NULL,
    purpose character varying(50),
    txn_time timestamp without time zone DEFAULT CURRENT_TIMESTAMP,
    stripe_payment_id character varying,
    error_message text
);


ALTER TABLE public.transaction_logs OWNER TO postgres;

--
-- TOC entry 223 (class 1259 OID 16494)
-- Name: transaction_logs_txn_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.transaction_logs_txn_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.transaction_logs_txn_id_seq OWNER TO postgres;

--
-- TOC entry 4933 (class 0 OID 0)
-- Dependencies: 223
-- Name: transaction_logs_txn_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.transaction_logs_txn_id_seq OWNED BY public.transaction_logs.txn_id;


--
-- TOC entry 220 (class 1259 OID 16475)
-- Name: vendors; Type: TABLE; Schema: public; Owner: postgres
--

CREATE TABLE public.vendors (
    vendor_id integer NOT NULL,
    shop_name character varying(100) NOT NULL,
    owner_name character varying(100)
);


ALTER TABLE public.vendors OWNER TO postgres;

--
-- TOC entry 219 (class 1259 OID 16474)
-- Name: vendors_vendor_id_seq; Type: SEQUENCE; Schema: public; Owner: postgres
--

CREATE SEQUENCE public.vendors_vendor_id_seq
    AS integer
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1;


ALTER SEQUENCE public.vendors_vendor_id_seq OWNER TO postgres;

--
-- TOC entry 4934 (class 0 OID 0)
-- Dependencies: 219
-- Name: vendors_vendor_id_seq; Type: SEQUENCE OWNED BY; Schema: public; Owner: postgres
--

ALTER SEQUENCE public.vendors_vendor_id_seq OWNED BY public.vendors.vendor_id;


--
-- TOC entry 4759 (class 2604 OID 16517)
-- Name: cards card_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.cards ALTER COLUMN card_id SET DEFAULT nextval('public.cards_card_id_seq'::regclass);


--
-- TOC entry 4757 (class 2604 OID 16471)
-- Name: students student_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.students ALTER COLUMN student_id SET DEFAULT nextval('public.students_stdent_id_seq'::regclass);


--
-- TOC entry 4760 (class 2604 OID 16498)
-- Name: transaction_logs txn_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.transaction_logs ALTER COLUMN txn_id SET DEFAULT nextval('public.transaction_logs_txn_id_seq'::regclass);


--
-- TOC entry 4758 (class 2604 OID 16478)
-- Name: vendors vendor_id; Type: DEFAULT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.vendors ALTER COLUMN vendor_id SET DEFAULT nextval('public.vendors_vendor_id_seq'::regclass);


--
-- TOC entry 4923 (class 0 OID 16482)
-- Dependencies: 222
-- Data for Name: cards; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.cards (card_id, student_id, balance) FROM stdin;
33BD8196	66200002	1100.00
33B8A5A9	66200001	955.00
E30D45A9	66200003	800.00
\.


--
-- TOC entry 4919 (class 0 OID 16468)
-- Dependencies: 218
-- Data for Name: students; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.students (student_id, full_name, class) FROM stdin;
66200001	bob	3
66200002	Tom	3
66200003	bai	3
\.


--
-- TOC entry 4925 (class 0 OID 16495)
-- Dependencies: 224
-- Data for Name: transaction_logs; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.transaction_logs (txn_id, card_id, vendor_id, amount, purpose, txn_time, stripe_payment_id, error_message) FROM stdin;
1	33BD8196	1	50.00	payment	2025-09-25 02:32:23.933328	\N	\N
2	33BD8196	1	25.00	payment	2025-09-25 02:32:53.779183	\N	\N
3	33BD8196	1	11.00	payment	2025-09-26 02:30:53.10031	\N	\N
4	33BD8196	1	11.00	payment	2025-09-26 02:31:55.829428	\N	\N
5	33BD8196	1	31.00	payment	2025-09-26 02:33:15.86276	\N	\N
6	33BD8196	1	31.00	payment	2025-09-26 02:36:32.512281	\N	\N
7	33BD8196	1	13.00	payment	2025-09-26 02:37:24.051785	\N	\N
8	33BD8196	1	31.00	payment	2025-09-26 02:39:27.752389	\N	\N
9	33BD8196	1	113.00	payment	2025-09-26 02:57:49.182593	\N	\N
10	33BD8196	1	33.00	payment	2025-09-26 02:59:48.94749	\N	\N
11	33BD8196	1	13.00	payment	2025-09-26 03:13:42.776006	\N	\N
12	33BD8196	1	11.00	payment	2025-09-26 03:28:54.049517	\N	\N
13	33BD8196	1	13.00	payment	2025-09-26 03:30:51.246492	\N	\N
14	33BD8196	1	33.00	payment	2025-09-26 23:40:18.779462	\N	\N
15	33BD8196	1	3.00	payment	2025-09-26 23:40:52.857043	\N	\N
16	33BD8196	1	1111.00	payment	2025-09-26 23:44:10.600233	\N	\N
17	33BD8196	1	1.00	payment	2025-09-26 23:44:54.700009	\N	\N
18	33BD8196	1	1.00	payment	2025-09-26 23:47:06.210708	\N	\N
19	33BD8196	1	1.00	payment	2025-09-26 23:52:22.867177	\N	\N
20	33BD8196	1	1111.00	payment	2025-09-26 23:55:00.060378	\N	\N
21	33BD8196	1	1.00	payment	2025-09-26 23:55:42.40887	\N	\N
22	33BD8196	1	3.00	payment	2025-09-26 23:56:13.05487	\N	\N
23	33BD8196	1	1.00	payment	2025-09-26 23:58:42.471271	\N	\N
24	33BD8196	1	3.00	payment	2025-09-27 00:43:55.775586	\N	\N
25	33BD8196	1	1.00	payment	2025-09-27 00:51:06.618961	\N	\N
26	33BD8196	1	3.00	payment	2025-09-27 00:51:28.701288	\N	\N
27	33BD8196	2	2.00	payment	2025-09-27 02:27:12.726724	\N	\N
28	33BD8196	2	2.00	payment	2025-09-27 02:27:17.917071	\N	\N
29	33BD8196	1	9.00	payment	2025-09-27 02:27:59.091455	\N	\N
30	33BD8196	1	9.00	payment	2025-09-27 02:28:02.656249	\N	\N
31	33BD8196	\N	1000.00	topup	2025-09-27 03:08:02.85594	\N	\N
32	33BD8196	1	1.00	payment	2025-09-27 03:09:03.048687	\N	\N
33	33BD8196	1	1.00	payment	2025-09-27 03:09:20.610104	\N	\N
34	33BD8196	1	9.00	payment	2025-09-27 03:15:34.388972	\N	\N
35	33BD8196	1	20.00	payment	2025-09-27 03:17:31.588926	\N	\N
36	33BD8196	1	30.00	payment	2025-09-27 03:17:44.199929	\N	\N
37	33BD8196	1	30.00	payment	2025-09-27 03:18:26.676091	\N	\N
38	33BD8196	1	1.00	payment	2025-09-28 15:57:02.316541	\N	\N
39	33BD8196	1	20.00	payment	2025-09-28 15:58:20.539977	\N	\N
40	33BD8196	1	20.00	payment	2025-09-28 16:21:33.819417	\N	\N
41	33BD8196	1	88.00	payment	2025-09-28 16:37:10.080096	\N	\N
42	33BD8196	1	69.00	payment	2025-09-28 17:38:53.761584	\N	\N
43	33BD8196	1	20.00	payment	2025-09-28 17:40:08.025543	\N	\N
44	33BD8196	\N	50.00	topup	2025-09-28 17:45:20.774153	\N	\N
45	33BD8196	1	20.00	payment	2025-09-28 18:26:29.334675	\N	\N
46	33BD8196	1	20.00	payment	2025-09-28 18:28:00.895507	\N	\N
47	33BD8196	\N	50.00	topup	2025-09-28 18:30:33.957013	\N	\N
48	33BD8196	1	20.00	payment	2025-09-28 20:16:07.853947	\N	\N
49	33BD8196	1	0.00	payment	2025-09-28 21:16:41.699426	\N	\N
50	33BD8196	\N	100.00	topup	2025-09-28 21:20:00.788227	\N	\N
\.


--
-- TOC entry 4921 (class 0 OID 16475)
-- Dependencies: 220
-- Data for Name: vendors; Type: TABLE DATA; Schema: public; Owner: postgres
--

COPY public.vendors (vendor_id, shop_name, owner_name) FROM stdin;
1	noodle_shop	tong
2	Stack_shop	fang
\.


--
-- TOC entry 4935 (class 0 OID 0)
-- Dependencies: 221
-- Name: cards_card_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.cards_card_id_seq', 1, false);


--
-- TOC entry 4936 (class 0 OID 0)
-- Dependencies: 217
-- Name: students_stdent_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.students_stdent_id_seq', 1, false);


--
-- TOC entry 4937 (class 0 OID 0)
-- Dependencies: 223
-- Name: transaction_logs_txn_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.transaction_logs_txn_id_seq', 50, true);


--
-- TOC entry 4938 (class 0 OID 0)
-- Dependencies: 219
-- Name: vendors_vendor_id_seq; Type: SEQUENCE SET; Schema: public; Owner: postgres
--

SELECT pg_catalog.setval('public.vendors_vendor_id_seq', 1, false);


--
-- TOC entry 4767 (class 2606 OID 16519)
-- Name: cards cards_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.cards
    ADD CONSTRAINT cards_pkey PRIMARY KEY (card_id);


--
-- TOC entry 4763 (class 2606 OID 16473)
-- Name: students students_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.students
    ADD CONSTRAINT students_pkey PRIMARY KEY (student_id);


--
-- TOC entry 4769 (class 2606 OID 16501)
-- Name: transaction_logs transaction_logs_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.transaction_logs
    ADD CONSTRAINT transaction_logs_pkey PRIMARY KEY (txn_id);


--
-- TOC entry 4765 (class 2606 OID 16480)
-- Name: vendors vendors_pkey; Type: CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.vendors
    ADD CONSTRAINT vendors_pkey PRIMARY KEY (vendor_id);


--
-- TOC entry 4771 (class 2606 OID 16534)
-- Name: transaction_logs fk_card_id; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.transaction_logs
    ADD CONSTRAINT fk_card_id FOREIGN KEY (card_id) REFERENCES public.cards(card_id);


--
-- TOC entry 4772 (class 2606 OID 16507)
-- Name: transaction_logs fk_vendor_id; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.transaction_logs
    ADD CONSTRAINT fk_vendor_id FOREIGN KEY (vendor_id) REFERENCES public.vendors(vendor_id);


--
-- TOC entry 4770 (class 2606 OID 16488)
-- Name: cards student_id; Type: FK CONSTRAINT; Schema: public; Owner: postgres
--

ALTER TABLE ONLY public.cards
    ADD CONSTRAINT student_id FOREIGN KEY (student_id) REFERENCES public.students(student_id);


-- Completed on 2025-09-28 21:53:00

--
-- PostgreSQL database dump complete
--

-- Completed on 2025-09-28 21:53:00

--
-- PostgreSQL database cluster dump complete
--

