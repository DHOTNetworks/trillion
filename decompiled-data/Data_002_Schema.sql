-- Database Schema Dump from: Data.002
-- Generated on: 9/21/2026 16:45:06

-- Total Tables: 148

CREATE TABLE [AuditTrail] (
    [Row_ID] INTEGER NULL,
    [ActivityDateTime] VARCHAR(255) NULL,
    [ActivityType] VARCHAR(255) NULL,
    [ActionType] VARCHAR(255) NULL,
    [VchType] VARCHAR(255) NULL,
    [VchDate] VARCHAR(255) NULL,
    [VchNo] VARCHAR(255) NULL,
    [LedgerName] VARCHAR(255) NULL,
    [DrAmount] VARCHAR(255) NULL,
    [CrAmount] VARCHAR(255) NULL,
    [Remarks] VARCHAR(255) NULL,
    [Others] VARCHAR(255) NULL
);

CREATE TABLE [BankAccounts] (
    [BankName] VARCHAR(255) NULL,
    [Code1st] INTEGER NULL,
    [BankBranchName] VARCHAR(255) NULL,
    [BSRCode] VARCHAR(255) NULL,
    [AccountNo] VARCHAR(255) NULL,
    [PostingLedger] INTEGER NULL,
    [AcPayeeLeftMargin] VARCHAR(255) NULL,
    [AcPayeeTopMargin] VARCHAR(255) NULL,
    [AcPayeeFontName] VARCHAR(255) NULL,
    [AcPayeeFontSize] VARCHAR(255) NULL,
    [AcPayeePrint] INTEGER NULL,
    [DateLeftMargin] VARCHAR(255) NULL,
    [DateTopMargin] VARCHAR(255) NULL,
    [DateFontName] VARCHAR(255) NULL,
    [DateFontSize] VARCHAR(255) NULL,
    [DatePrint] INTEGER NULL,
    [PartyNameLeftMargin] VARCHAR(255) NULL,
    [PartyNameTopMargin] VARCHAR(255) NULL,
    [PartyNameFontName] VARCHAR(255) NULL,
    [PartyNameFontSize] VARCHAR(255) NULL,
    [PartyNamePrint] INTEGER NULL,
    [AmountWordsLeftMargin] VARCHAR(255) NULL,
    [AmountWordsTopMargin] VARCHAR(255) NULL,
    [AmountWordsFontName] VARCHAR(255) NULL,
    [AmountWordsFontSize] VARCHAR(255) NULL,
    [AmountWordsPrint] INTEGER NULL,
    [AmountFiguresLeftMargin] VARCHAR(255) NULL,
    [AmountFiguresTopMargin] VARCHAR(255) NULL,
    [AmountFiguresFontName] VARCHAR(255) NULL,
    [AmountFiguresFontSize] VARCHAR(255) NULL,
    [AmountFiguresPrint] INTEGER NULL,
    [BankAcLeftMargin] VARCHAR(255) NULL,
    [BankAcTopMargin] VARCHAR(255) NULL,
    [BankAcFontName] VARCHAR(255) NULL,
    [BankAcFontSize] VARCHAR(255) NULL,
    [BankAcPrint] INTEGER NULL,
    [BankBranchLeftMargin] VARCHAR(255) NULL,
    [BankBranchTopMargin] VARCHAR(255) NULL,
    [BankBranchFontName] VARCHAR(255) NULL,
    [BankBranchFontSize] VARCHAR(255) NULL,
    [BankBranchPrint] INTEGER NULL,
    [StampLeftMargin] VARCHAR(255) NULL,
    [StampTopMargin] VARCHAR(255) NULL,
    [StampFontName] VARCHAR(255) NULL,
    [StampFontSize] VARCHAR(255) NULL,
    [StampPrint] INTEGER NULL,
    [StampFor] VARCHAR(255) NULL,
    [StampAuthTopMargin] VARCHAR(255) NULL,
    [StampAuth] VARCHAR(255) NULL,
    [PaperLayout] VARCHAR(255) NULL
);

CREATE TABLE [BardanaTransactions] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [VchType] VARCHAR(255) NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [AccountCode] INTEGER NULL,
    [Qty] VARCHAR(255) NULL,
    [VehNo] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [BikriIssueVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [RowNo] INTEGER NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [AccountCode] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [Bags] DOUBLE NULL,
    [Packing] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [TaxRate] REAL NULL,
    [SurchargeRate] REAL NULL,
    [Amount] DOUBLE NULL,
    [ExpType] VARCHAR(255) NULL,
    [ExpRate] DOUBLE NULL,
    [ExpUnit] VARCHAR(255) NULL,
    [PartyBillNo] VARCHAR(255) NULL,
    [PartyBillDate] VARCHAR(255) NULL,
    [PartyGRNo] VARCHAR(255) NULL,
    [PartyVehicleNo] VARCHAR(255) NULL,
    [PartyWeight] VARCHAR(255) NULL,
    [PartyBillAmount] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [TypeofTransport] VARCHAR(255) NULL,
    [TransportName] VARCHAR(255) NULL,
    [TypeofReceipt] VARCHAR(255) NULL,
    [ReceiptNo] VARCHAR(255) NULL,
    [DateofReceipt] VARCHAR(255) NULL,
    [DeliveryDate] VARCHAR(255) NULL
);

CREATE TABLE [BillImageMargins] (
    [ImageType] VARCHAR(255) NULL,
    [ReportName] VARCHAR(255) NULL,
    [TopMargin] INTEGER NULL,
    [LeftMargin] INTEGER NULL,
    [ImageHeight] INTEGER NULL,
    [ImageWidth] INTEGER NULL
);

CREATE TABLE [BillSeries] (
    [RowNo] INTEGER NULL,
    [BillPrefix] VARCHAR(255) NULL,
    [VchType] VARCHAR(255) NULL
);

CREATE TABLE [BillWiseReceiptVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [PartyCode] INTEGER NULL,
    [RowNo] INTEGER NULL,
    [Amount] DOUBLE NULL,
    [Discount] DOUBLE NULL,
    [Interest] DOUBLE NULL,
    [VchMode] VARCHAR(255) NULL,
    [DrCrLedger] INTEGER NULL,
    [ChqNo] VARCHAR(255) NULL,
    [Remarks] VARCHAR(255) NULL,
    [BillVoucherDate] DATETIME NULL,
    [BillVoucherNumber] INTEGER NULL,
    [BillTransType] VARCHAR(255) NULL
);

CREATE TABLE [BooksSettings] (
    [BookType] VARCHAR(50) NULL,
    [ChPt] BOOLEAN NOT NULL,
    [ChRt] BOOLEAN NOT NULL,
    [Pymt] BOOLEAN NOT NULL,
    [Rcpt] BOOLEAN NOT NULL,
    [Jrnl] BOOLEAN NOT NULL,
    [Sale] BOOLEAN NOT NULL,
    [Purc] BOOLEAN NOT NULL,
    [JFrm] BOOLEAN NOT NULL,
    [IFrm] BOOLEAN NOT NULL,
    [Self] BOOLEAN NOT NULL,
    [SlRn] BOOLEAN NOT NULL,
    [PrRn] BOOLEAN NOT NULL,
    [BookCaption] VARCHAR(50) NULL,
    [CaptionLanguage] VARCHAR(8) NULL,
    [ShowBillItems] INTEGER NULL,
    [ShowZimidarName] INTEGER NULL,
    [ShowMaalKhataDetail] INTEGER NULL,
    [ShowIFrmDetail] INTEGER NULL,
    [NormalFontPrinting] INTEGER NULL,
    [ShowPurcDetail] INTEGER NULL,
    [ShowCashCreditSeperate] INTEGER NULL,
    [PrintShortFormat] INTEGER NULL,
    [ShowZeroAmountVouchersInLedger] INTEGER NULL,
    [ShowBothQtyInCashBook] INTEGER NULL,
    [TopMarginForReports] DOUBLE NULL
);

CREATE TABLE [BrokerageVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [TransType] VARCHAR(255) NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [PartyCode] INTEGER NULL,
    [CrLedger] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [ItemRemarks] VARCHAR(255) NULL,
    [ItemQty] DOUBLE NULL,
    [ItemWeight] DOUBLE NULL,
    [ItemRate] DOUBLE NULL,
    [ItemAmount] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [RateUnit] VARCHAR(255) NULL,
    [Amount] DOUBLE NULL,
    [DrLedger] INTEGER NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [ChallanVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [RowNo] INTEGER NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [AccountCode] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [ItemName] VARCHAR(255) NULL,
    [Bags] DOUBLE NULL,
    [Packing] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [TaxRate] REAL NULL,
    [SurchargeRate] REAL NULL,
    [Amount] DOUBLE NULL,
    [ExpType] VARCHAR(255) NULL,
    [ExpRate] DOUBLE NULL,
    [ExpUnit] VARCHAR(255) NULL,
    [PartyBillNo] VARCHAR(255) NULL,
    [PartyBillDate] VARCHAR(255) NULL,
    [PartyGRNo] VARCHAR(255) NULL,
    [PartyVehicleNo] VARCHAR(255) NULL,
    [PartyWeight] VARCHAR(255) NULL,
    [PartyBillAmount] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [ChallanHeading] VARCHAR(255) NULL,
    [HSNCode] VARCHAR(255) NULL
);

CREATE TABLE [ChequesList] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [BankCode] INTEGER NULL,
    [RowNo] INTEGER NULL,
    [PartyCode] INTEGER NULL,
    [Amount] DOUBLE NULL,
    [ChequeNo] VARCHAR(255) NULL,
    [PrintOtherName] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [OtherAccountCode] INTEGER NULL,
    [DrCr1] VARCHAR(255) NULL,
    [Amount1] VARCHAR(255) NULL,
    [Narration1] VARCHAR(255) NULL
);

CREATE TABLE [CommissionBasisItemOpeningStock] (
    [PartyCode] SMALLINT NULL,
    [ItemCode] SMALLINT NULL,
    [Bags] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [AccountCode] SMALLINT NULL,
    [Amount] DOUBLE NULL
);

CREATE TABLE [CommissionBasisItemStock] (
    [PartyCode] SMALLINT NULL,
    [ItemCode] SMALLINT NULL,
    [Bags] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [AccountCode] SMALLINT NULL,
    [Amount] DOUBLE NULL
);

CREATE TABLE [CompanyInfo] (
    [CompanyName] VARCHAR(255) NULL,
    [Business] VARCHAR(250) NOT NULL,
    [AccYearFrom] DATETIME NOT NULL,
    [AccYearTo] DATETIME NOT NULL,
    [BooksBeginingFrom] DATETIME NULL,
    [Address] VARCHAR(250) NOT NULL,
    [Phone_O] VARCHAR(250) NOT NULL,
    [Phone_R] VARCHAR(250) NOT NULL,
    [Phone_F] VARCHAR(250) NOT NULL,
    [Mobile1] VARCHAR(250) NOT NULL,
    [Mobile2] VARCHAR(250) NOT NULL,
    [Fax] VARCHAR(250) NOT NULL,
    [CST_No] VARCHAR(250) NOT NULL,
    [TIN_No] VARCHAR(250) NOT NULL,
    [PAN_No] VARCHAR(250) NOT NULL,
    [Jurisdiction] VARCHAR(250) NOT NULL,
    [Passwd] VARCHAR(50) NOT NULL,
    [HFGA_No] VARCHAR(250) NULL,
    [ML_No] VARCHAR(250) NULL,
    [Cotton_No] VARCHAR(250) NULL,
    [TDS_Circle] VARCHAR(250) NULL,
    [MySTATE] VARCHAR(255) NULL,
    [MyBUSINESS] VARCHAR(255) NULL,
    [MyStation] VARCHAR(255) NULL,
    [CompositionScheme] INTEGER NULL,
    [Bank2] VARCHAR(255) NULL,
    [DATABreakNarr] VARCHAR(255) NULL,
    [RegdNo] VARCHAR(255) NULL,
    [Bank3] VARCHAR(255) NULL,
    [FirmType] VARCHAR(255) NULL,
    [FirmShortName] VARCHAR(255) NULL,
    [TINDated] VARCHAR(255) NULL,
    [SaleTaxOffice] VARCHAR(255) NULL,
    [GSTIN] VARCHAR(255) NULL,
    [SyncEnabled] INTEGER NULL,
    [URN] VARCHAR(255) NULL,
    [BRN] VARCHAR(255) NULL
);

CREATE TABLE [CottonBalesDetail] (
    [U_ID] VARCHAR(255) NULL,
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [RowNo] INTEGER NOT NULL,
    [Col1] DOUBLE NULL,
    [Col2] DOUBLE NULL,
    [Col3] DOUBLE NULL,
    [Col4] DOUBLE NULL,
    [Col5] DOUBLE NULL,
    [Col6] DOUBLE NULL,
    [Col7] DOUBLE NULL,
    [Col8] DOUBLE NULL,
    [Col9] DOUBLE NULL,
    [Col10] DOUBLE NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CottonVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [PartyCode] INTEGER NULL,
    [PrintName] VARCHAR(255) NULL,
    [PrintAddress] VARCHAR(255) NULL,
    [TIN] VARCHAR(255) NULL,
    [PackingType] VARCHAR(255) NULL,
    [BalesFrom] VARCHAR(255) NULL,
    [BalesTo] VARCHAR(255) NULL,
    [Qty] VARCHAR(255) NULL,
    [QtyPackingType] VARCHAR(255) NULL,
    [ItemCode] INTEGER NULL,
    [ItemVAT] DOUBLE NULL,
    [ItemCST] DOUBLE NULL,
    [GrossWeight] DOUBLE NULL,
    [TareWeight] DOUBLE NULL,
    [ShortageWeight] DOUBLE NULL,
    [NetWeight] DOUBLE NULL,
    [RateUnit] VARCHAR(255) NULL,
    [RatePerMaund] DOUBLE NULL,
    [RatePerQtl] DOUBLE NULL,
    [PressMark] VARCHAR(255) NULL,
    [PressDate] VARCHAR(255) NULL,
    [DueDate] VARCHAR(255) NULL,
    [LOTNo] VARCHAR(255) NULL,
    [VehicleNo] VARCHAR(255) NULL,
    [GRNo] VARCHAR(255) NULL,
    [Broker] VARCHAR(255) NULL,
    [ST38No] VARCHAR(255) NULL,
    [DeliveryFrom] VARCHAR(255) NULL,
    [DeliveryTo] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [Narr1] VARCHAR(255) NULL,
    [Narr2] VARCHAR(255) NULL,
    [Narr3] VARCHAR(255) NULL,
    [Narr4] VARCHAR(255) NULL,
    [Narr5] VARCHAR(255) NULL,
    [Narr6] VARCHAR(255) NULL,
    [TransType] VARCHAR(255) NULL,
    [DiscountRate] DOUBLE NULL,
    [TraceRate] DOUBLE NULL,
    [OtherExp1Type] VARCHAR(255) NULL,
    [OtherExp2Type] VARCHAR(255) NULL,
    [Pkng] VARCHAR(255) NULL,
    [DueDateDays] VARCHAR(255) NULL,
    [CommissionRate] VARCHAR(255) NULL,
    [MarketFeeRate] REAL NULL,
    [HRDFRate] REAL NULL,
    [ShippingAddress] VARCHAR(255) NULL,
    [MktCmtSrNoNew] VARCHAR(255) NULL,
    [QRCode] BLOB NULL,
    [IRNNo] VARCHAR(255) NULL,
    [TransporterName] VARCHAR(255) NULL,
    [TransporterGSTIN] VARCHAR(255) NULL,
    [TransportMode] VARCHAR(255) NULL,
    [Distance] VARCHAR(255) NULL,
    [TransportDocNo] VARCHAR(255) NULL,
    [TransportDocDt] VARCHAR(255) NULL,
    [BuyerLocation] VARCHAR(255) NULL,
    [BuyerPINCode] VARCHAR(255) NULL,
    [EWayOthers] VARCHAR(255) NULL,
    [ShipFrom_GSTIN] VARCHAR(255) NULL,
    [ShipFrom_Nm] VARCHAR(255) NULL,
    [ShipFrom_Addr1] VARCHAR(255) NULL,
    [ShipFrom_Loc] VARCHAR(255) NULL,
    [ShipFrom_Pin] VARCHAR(255) NULL,
    [ShipFrom_Stcd] VARCHAR(255) NULL,
    [ShipFrom_Others] VARCHAR(255) NULL,
    [ShipTo_Gstin] VARCHAR(255) NULL,
    [ShipTo_Nm] VARCHAR(255) NULL,
    [ShipTo_Addr1] VARCHAR(255) NULL,
    [ShipTo_Loc] VARCHAR(255) NULL,
    [ShipTo_Pin] VARCHAR(255) NULL,
    [ShipTo_Stcd] VARCHAR(255) NULL,
    [ShipTo_Others] VARCHAR(255) NULL,
    [EInvTransType] VARCHAR(255) NULL,
    [EInvStatus] VARCHAR(255) NULL,
    [EWayStatus] VARCHAR(255) NULL,
    [TransportNm] VARCHAR(255) NULL,
    [BalesUID] VARCHAR(255) NULL,
    [BardanaType] VARCHAR(255) NULL,
    [SaleAccountCode] INTEGER NULL,
    [EInvAckNo] VARCHAR(255) NULL,
    [EInvAckDate] VARCHAR(255) NULL
);

CREATE TABLE [CustomClosingStocks] (
    [ClosingStockDate] DATETIME NULL,
    [ItemCode] SMALLINT NULL,
    [Bags] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [Amount] DOUBLE NULL
);

CREATE TABLE [CustomNarrationForChPt] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForChRt] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForDrCr] (
    [Narration] VARCHAR(255) NULL,
    [F1] VARCHAR(255) NULL,
    [F2] VARCHAR(255) NULL,
    [F3] VARCHAR(255) NULL,
    [F4] VARCHAR(255) NULL,
    [F5] VARCHAR(255) NULL,
    [F6] VARCHAR(255) NULL,
    [F7] VARCHAR(255) NULL,
    [F8] VARCHAR(255) NULL,
    [F9] VARCHAR(255) NULL,
    [F10] VARCHAR(255) NULL,
    [F11] VARCHAR(255) NULL,
    [F12] VARCHAR(255) NULL,
    [F13] VARCHAR(255) NULL,
    [F14] VARCHAR(255) NULL,
    [F15] VARCHAR(255) NULL,
    [F16] TEXT NULL,
    [F17] VARCHAR(255) NULL,
    [F18] VARCHAR(255) NULL,
    [F19] VARCHAR(255) NULL,
    [F20] DATETIME NULL,
    [F21] VARCHAR(255) NULL,
    [F22] VARCHAR(255) NULL,
    [F23] VARCHAR(255) NULL,
    [F24] VARCHAR(255) NULL,
    [F25] VARCHAR(255) NULL,
    [F26] DATETIME NULL
);

CREATE TABLE [CustomNarrationForIFrm] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForJFrm] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForJrnl] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForMemo] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForPurc] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForPymt] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForRcpt] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [CustomNarrationForSale] (
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [DebitCreditNotes] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [NoteType] VARCHAR(255) NULL,
    [NoteMode] VARCHAR(255) NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [BillNo] VARCHAR(255) NULL,
    [BillDate] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [Bags] VARCHAR(255) NULL,
    [Weight] VARCHAR(255) NULL,
    [Rate] VARCHAR(255) NULL,
    [Amount] VARCHAR(255) NULL,
    [Interest] VARCHAR(255) NULL,
    [VehicleNo] VARCHAR(255) NULL,
    [Transport] VARCHAR(255) NULL,
    [Broker] VARCHAR(255) NULL,
    [Others] VARCHAR(255) NULL,
    [ChqAmount] VARCHAR(255) NULL,
    [ChqNo] VARCHAR(255) NULL,
    [CashAmount] VARCHAR(255) NULL,
    [Remarks] VARCHAR(255) NULL,
    [SGSTRate] VARCHAR(255) NULL,
    [CGSTRate] VARCHAR(255) NULL,
    [IGSTRate] VARCHAR(255) NULL,
    [CessRate] VARCHAR(255) NULL,
    [Reason] VARCHAR(255) NULL,
    [ActualInv] INTEGER NULL,
    [QRCode] BLOB NULL,
    [IRNNo] VARCHAR(255) NULL,
    [EInvStatus] VARCHAR(255) NULL,
    [EInvAckNo] VARCHAR(255) NULL,
    [EInvAckDate] VARCHAR(255) NULL,
    [PrintasDebitNoteIssue] INTEGER NULL,
    [DebitNoteIssueNo] VARCHAR(255) NULL
);

CREATE TABLE [DeliveryChallans] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [AgainstInvoice] VARCHAR(255) NULL,
    [PartyCode] DOUBLE NULL,
    [TIN] VARCHAR(255) NULL,
    [City] VARCHAR(255) NULL,
    [ItemCode] DOUBLE NULL,
    [BrandName] VARCHAR(255) NULL,
    [Bags] DOUBLE NULL,
    [Packing] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [TruckNo] VARCHAR(255) NULL,
    [GRNo] VARCHAR(255) NULL,
    [ChallanTime] VARCHAR(255) NULL,
    [ST38No] VARCHAR(255) NULL,
    [DriverName] VARCHAR(255) NULL,
    [TransportCo] VARCHAR(255) NULL,
    [FreightRate] DOUBLE NULL,
    [ServiceTaxRate] DOUBLE NULL,
    [TDSRate] DOUBLE NULL,
    [TransportCoPAN] VARCHAR(255) NULL,
    [GodownCode] INTEGER NULL,
    [ChallanNo] VARCHAR(255) NULL
);

CREATE TABLE [EmptyTransactions] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [AccountCode] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [Ctr] DOUBLE NULL,
    [Bottles] DOUBLE NULL,
    [DrCr] VARCHAR(255) NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [FormsNames] (
    [FormName] VARCHAR(255) NULL,
    [Code1st] INTEGER NULL,
    [OpStock] INTEGER NULL,
    [Narration] VARCHAR(255) NULL,
    [CurrentStock] INTEGER NULL,
    [AutoSplitUpdate] INTEGER NULL
);

CREATE TABLE [FormsVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [VoucherType] VARCHAR(255) NULL,
    [FormCode] INTEGER NULL,
    [PartyType] VARCHAR(255) NULL,
    [PartyCode] INTEGER NULL,
    [Qty] INTEGER NULL,
    [AmountType] VARCHAR(255) NULL,
    [FormAmount] DOUBLE NULL,
    [FormNo] VARCHAR(255) NULL,
    [FormDate] VARCHAR(255) NULL,
    [PersonName] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [CancelFormNo] VARCHAR(255) NULL,
    [Commodity] VARCHAR(255) NULL,
    [FormPeriod] VARCHAR(255) NULL,
    [PartyName] VARCHAR(255) NULL,
    [OrderNo] VARCHAR(100) NULL,
    [BLNo] VARCHAR(100) NULL,
    [PortVessel] VARCHAR(100) NULL
);

CREATE TABLE [FormsVouchersDetails] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [VoucherType] VARCHAR(255) NULL,
    [BillVoucherDate] DATETIME NULL,
    [BillVoucherNumber] INTEGER NULL,
    [BillTransType] VARCHAR(255) NULL,
    [BillInvoiceNo] VARCHAR(255) NULL,
    [BillItem] VARCHAR(255) NULL,
    [BillAmount] DOUBLE NULL
);

CREATE TABLE [GatePassVouchers] (
    [GatePassDate] DATETIME NULL,
    [GatePassNo] INTEGER NULL,
    [PartyCode] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [IFrmNo] INTEGER NULL,
    [Bags] DOUBLE NULL,
    [Pkng] REAL NULL,
    [Weight] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [VehNo] VARCHAR(255) NULL,
    [DriverName] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [VehType] VARCHAR(255) NULL,
    [TotalWeight] DOUBLE NULL,
    [FreightRateUnit] VARCHAR(255) NULL,
    [FreightRate] DOUBLE NULL,
    [FreightAmount] DOUBLE NULL,
    [AmountPaid] DOUBLE NULL,
    [RowNo] INTEGER NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [GRNo] VARCHAR(255) NULL,
    [GodownName] VARCHAR(255) NULL,
    [ItemPkng] DOUBLE NULL
);

CREATE TABLE [GateRegister] (
    [VoucherDate] DATETIME NULL,
    [RowNo] INTEGER NULL,
    [PartyName] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [Qty] VARCHAR(255) NULL,
    [DocNo] VARCHAR(255) NULL,
    [Remarks] VARCHAR(255) NULL
);

CREATE TABLE [Groups] (
    [GroupName] VARCHAR(50) NULL,
    [Code1st] SMALLINT NOT NULL,
    [Code2nd] SMALLINT NOT NULL,
    [Code3rd] SMALLINT NOT NULL,
    [Code4th] SMALLINT NOT NULL,
    [ExtractInBalanceSheet] DOUBLE NULL,
    [MFeeLastPrintFromDATE] DATETIME NULL,
    [MFeeLastPrintToDATE] DATETIME NULL,
    [LastMFeeAmount] DOUBLE NULL,
    [HRDFLastPrintFromDATE] DATETIME NULL,
    [HRDFLastPrintToDATE] DATETIME NULL,
    [LastHRDFAmount] DOUBLE NULL,
    [SyncEnabled] INTEGER NULL
);

CREATE TABLE [GSTRates] (
    [RowNo] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [DateFrom] DATETIME NULL,
    [SGST] DOUBLE NULL,
    [CGST] DOUBLE NULL,
    [IGST] DOUBLE NULL,
    [CESS] DOUBLE NULL,
    [OtherCess] DOUBLE NULL,
    [TempMark] INTEGER NULL,
    [MarkDt] DATETIME NULL
);

CREATE TABLE [InterestCDSettings] (
    [RowNo] INTEGER NULL,
    [Days] INTEGER NULL,
    [CDPaisa] DOUBLE NULL,
    [InterestType] VARCHAR(255) NULL
);

CREATE TABLE [InterestSettings] (
    [InterestType] VARCHAR(50) NULL,
    [NormalRateofInterest] REAL NULL,
    [RateAfterDueDays] REAL NULL,
    [DueDays] SMALLINT NULL,
    [YearDays] SMALLINT NULL,
    [PermanentGrace] BOOLEAN NOT NULL,
    [DrSideInterestRate] DOUBLE NULL,
    [DayForDropDays] VARCHAR(255) NULL,
    [DropDays] DOUBLE NULL,
    [MakeProduct] DOUBLE NULL,
    [YearDaysDivideBy] DOUBLE NULL,
    [FIRSTorLASTDayCount] DOUBLE NULL,
    [InterestNotUptoDays] INTEGER NULL,
    [OpeningAmountInclude] DOUBLE NULL,
    [IFrmDamiTDSDeduct] INTEGER NULL,
    [UseSaudaDate] INTEGER NULL,
    [ShowDiscDays] INTEGER NULL,
    [ShowDiscAank] INTEGER NULL,
    [ShowAankRoundoff] INTEGER NULL,
    [OtherInterestMode] INTEGER NULL,
    [AddLastDay] VARCHAR(255) NULL,
    [ClosingAsPayment] INTEGER NULL,
    [NormalRateofInterestCr] DOUBLE NULL,
    [RateAfterDueDaysCr] DOUBLE NULL,
    [RateForOpBal] DOUBLE NULL,
    [RateAfterOtherDays] DOUBLE NULL,
    [OtherDays] INTEGER NULL,
    [CountFullMonthAfterDays] INTEGER NULL,
    [PromptDrCrDates] INTEGER NULL,
    [ShowNarrWiseTotal] INTEGER NULL
);

CREATE TABLE [ItemOpeningStockDetails] (
    [ItemCode] INTEGER NULL,
    [Pcs] DOUBLE NULL,
    [Unit] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [BatchNo] VARCHAR(255) NULL,
    [ExpiryDate] VARCHAR(255) NULL
);

CREATE TABLE [ItemRateSlabs] (
    [RowNo] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [DateFrom] DATETIME NULL,
    [DateTo] DATETIME NULL,
    [PurcRate] DOUBLE NULL,
    [SaleRate] DOUBLE NULL
);

CREATE TABLE [ItemStockDetails] (
    [ItemCode] INTEGER NULL,
    [Pcs] INTEGER NULL,
    [BatchNo] VARCHAR(255) NULL,
    [ExpiryDate] DATETIME NULL,
    [PurcDate] DATETIME NULL,
    [RowNo] INTEGER NULL
);

CREATE TABLE [JFrmSettings] (
    [GoodsAmountRoundOff] BOOLEAN NOT NULL,
    [LabourRoundOff] BOOLEAN NOT NULL,
    [BonusRoundOff] BOOLEAN NOT NULL,
    [ReliefRoundOff] BOOLEAN NOT NULL,
    [Roundoff] BOOLEAN NOT NULL,
    [MarketFeeLastPrintingDate] TEXT NULL,
    [HRDFLastPrintingDate] TEXT NULL,
    [PartiesPaymentLastPrintingDate] VARCHAR(255) NULL,
    [PaymentSummaryLastPrintingDate] VARCHAR(255) NULL,
    [MFeeLastPrintFromDATE] DATETIME NULL,
    [MFeeLastPrintToDATE] DATETIME NULL,
    [LastMFeeAmount] DOUBLE NULL,
    [HRDFLastPrintFromDATE] DATETIME NULL,
    [HRDFLastPrintToDATE] DATETIME NULL,
    [LastHRDFAmount] DOUBLE NULL
);

CREATE TABLE [JournalStockTransactions] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [ItemCode] DOUBLE NULL,
    [DrCr] VARCHAR(255) NULL,
    [Bags] DOUBLE NULL,
    [Packing] REAL NULL,
    [Weight] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [ItemType] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [VATCST] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [DheriPurchaseDate] DATETIME NULL,
    [DheriPurchaseFrom] INTEGER NULL,
    [DheriBillNo] VARCHAR(255) NULL,
    [DheriNo] INTEGER NULL,
    [VoucherType] VARCHAR(255) NULL,
    [CommissionPartyCode] INTEGER NULL,
    [ItemRowNo] INTEGER NULL,
    [Percentag] VARCHAR(255) NULL
);

CREATE TABLE [LagatBikriVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [VoucherType] VARCHAR(255) NULL,
    [Bags] VARCHAR(255) NULL,
    [Weight] VARCHAR(255) NULL,
    [Rate] VARCHAR(255) NULL,
    [Amount] VARCHAR(255) NULL,
    [Remarks] VARCHAR(255) NULL,
    [BillNo] INTEGER NULL,
    [BillDateFrom] VARCHAR(255) NULL,
    [BillDateTo] VARCHAR(255) NULL,
    [BillItemCode] INTEGER NULL,
    [BillBags] DOUBLE NULL,
    [BillWeight] DOUBLE NULL,
    [BillAmount] DOUBLE NULL,
    [PartyAccountCode] INTEGER NULL,
    [ItemCode] INTEGER NULL
);

CREATE TABLE [LedgerCreationSettings] (
    [RouteUsed] BOOLEAN NOT NULL,
    [LedgerNameAutoTitleCase] DOUBLE NULL
);

CREATE TABLE [Ledgers] (
    [Prefix] VARCHAR(15) NULL,
    [LedgerName] VARCHAR(255) NULL,
    [LedgerAlais] VARCHAR(255) NOT NULL,
    [Code1st] SMALLINT NOT NULL,
    [GroupCode] SMALLINT NOT NULL,
    [InventoryChoice] VARCHAR(50) NOT NULL,
    [OpeningBal] DOUBLE NULL,
    [OpeningType] VARCHAR(2) NOT NULL,
    [MailingName] VARCHAR(250) NOT NULL,
    [MailingAdd] VARCHAR(255) NOT NULL,
    [IncomeTaxNo] VARCHAR(50) NOT NULL,
    [SaleTaxNo] VARCHAR(50) NOT NULL,
    [VAT] VARCHAR(50) NOT NULL,
    [Station] VARCHAR(50) NULL,
    [Route] VARCHAR(100) NULL,
    [Distt] VARCHAR(100) NULL,
    [CurrentBalance] VARCHAR(50) NULL,
    [Percentage] DOUBLE NULL,
    [InterestRate] DOUBLE NULL,
    [SalaryPerMonth] DOUBLE NULL,
    [BankAccount] VARCHAR(255) NULL,
    [STATE] VARCHAR(255) NULL,
    [ShowDateTotals] INTEGER NULL,
    [AutoSplitUpdate] INTEGER NULL,
    [PartyTAN] VARCHAR(255) NULL,
    [PartyType] VARCHAR(255) NULL,
    [ConcernedPerson] VARCHAR(255) NULL,
    [PartyState] VARCHAR(255) NULL,
    [PartyPINcode] VARCHAR(255) NULL,
    [VATDealer] VARCHAR(255) NULL,
    [PartyStation] VARCHAR(255) NULL,
    [CreditLimit] DOUBLE NULL,
    [MobNoForSMS] VARCHAR(255) NULL,
    [CalculateInJointTrading] INTEGER NULL,
    [SpecialPartyType] VARCHAR(255) NULL,
    [ShopNo] VARCHAR(255) NULL,
    [CommnCalcOn] VARCHAR(255) NULL,
    [Email] VARCHAR(255) NULL,
    [StockNotCalculateInLedger] INTEGER NULL,
    [GSTIN] VARCHAR(255) NULL,
    [AadharNo] VARCHAR(255) NULL,
    [GSTPartyType] VARCHAR(255) NULL,
    [WhatsappNo] VARCHAR(255) NULL,
    [SyncEnabled] INTEGER NULL,
    [IFSCCode] VARCHAR(255) NULL,
    [BankName] VARCHAR(255) NULL,
    [ApplyTCSForParty] INTEGER NULL,
    [BRN] VARCHAR(255) NULL,
    [URN] VARCHAR(255) NULL,
    [TCSNotApplyInSale] INTEGER NULL
);

CREATE TABLE [LedgerVerifications] (
    [VerifyDate] DATETIME NULL,
    [LedgerCode] INTEGER NULL,
    [Comment] TEXT NULL
);

CREATE TABLE [ListForSMS] (
    [RowNo] INTEGER NULL,
    [MobNo] VARCHAR(255) NULL,
    [SMSText] VARCHAR(255) NULL,
    [Status] INTEGER NULL,
    [Template_ID] VARCHAR(255) NULL,
    [Entity_ID] VARCHAR(255) NULL,
    [Attachmt] VARCHAR(255) NULL
);

CREATE TABLE [LLFormReceiptNo] (
    [RowNo] INTEGER NULL,
    [FormType] VARCHAR(255) NULL,
    [RcptDate] DATETIME NULL,
    [RcptNo] VARCHAR(255) NULL,
    [RcptAmount] DOUBLE NULL,
    [TickRcpt] INTEGER NULL
);

CREATE TABLE [ManualVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [TransType] VARCHAR(255) NULL,
    [VchType] VARCHAR(255) NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [CrLedger] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [ItemName] VARCHAR(255) NULL,
    [RateUnit] VARCHAR(255) NULL,
    [Qty] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [DrLedger] INTEGER NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [MemoRegister] (
    [RowNo] SMALLINT NULL,
    [VoucherNumber] INTEGER NULL,
    [VoucherDate] DATETIME NULL,
    [AccountCode] SMALLINT NOT NULL,
    [DrCr] VARCHAR(3) NOT NULL,
    [Amount] DOUBLE NOT NULL,
    [Narration] VARCHAR(200) NOT NULL
);

CREATE TABLE [MilkRateList] (
    [FAT] DOUBLE NULL,
    [P60] DOUBLE NULL
);

CREATE TABLE [MilkRateListNew] (
    [FAT] DOUBLE NULL,
    [SNF] DOUBLE NULL,
    [Rate] DOUBLE NULL
);

CREATE TABLE [MillingVouchers] (
    [RowNo] DOUBLE NULL,
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [ItemCode] DOUBLE NULL,
    [DrCr] VARCHAR(255) NULL,
    [Percentage] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [Bags] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [Narrtn] VARCHAR(255) NULL
);

CREATE TABLE [MTVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [RowNo] INTEGER NULL,
    [SrNo] VARCHAR(255) NULL,
    [BillDate] VARCHAR(255) NULL,
    [PartyName] VARCHAR(255) NULL,
    [BillNo] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [Qty] VARCHAR(255) NULL,
    [BillAmount] VARCHAR(255) NULL,
    [Muddat] VARCHAR(255) NULL,
    [TDS] VARCHAR(255) NULL,
    [NetAmount] VARCHAR(255) NULL,
    [VchNo] VARCHAR(255) NULL,
    [TransType] VARCHAR(255) NULL,
    [MuddatDays] VARCHAR(255) NULL,
    [InterestRate] VARCHAR(255) NULL,
    [TransPartyWise] INTEGER NULL,
    [BankAccountCode] INTEGER NULL,
    [ChqNo] VARCHAR(255) NULL,
    [FromDate] DATETIME NULL,
    [ToDate] DATETIME NULL,
    [LdgrCode] INTEGER NULL,
    [DamiTDS] DOUBLE NULL,
    [InttTDS] DOUBLE NULL,
    [MuddatDate] VARCHAR(255) NULL,
    [GSTPymt] INTEGER NULL,
    [TDS194Q] DOUBLE NULL
);

CREATE TABLE [NewTable] (
    [ID] INTEGER NOT NULL,
    [VoucherNo] INTEGER NULL,
    [VoucherType] VARCHAR(255) NULL,
    [details] TEXT NULL,
    [vdate] DATETIME NULL
);

CREATE TABLE [Notes] (
    [RowNo] INTEGER NULL,
    [Notes] TEXT NULL
);

CREATE TABLE [OtherLedgers] (
    [DailyBalanceLedger1] DOUBLE NULL,
    [DailyBalanceLedger2] DOUBLE NULL,
    [MonthlyBalanceLedger1] DOUBLE NULL,
    [MonthlyBalanceLedger2] DOUBLE NULL,
    [MonthlyBalanceLedger3] DOUBLE NULL,
    [YearlyBalanceLedger1] DOUBLE NULL,
    [YearlyBalanceLedger2] DOUBLE NULL,
    [YearlyBalanceLedger3] DOUBLE NULL
);

CREATE TABLE [OtherSettings] (
    [ShowLedgerVerificationsInLedgerList] INTEGER NULL,
    [LabourPerNag] INTEGER NULL,
    [LabourRateBag] VARCHAR(255) NULL,
    [LabourRateQtl] VARCHAR(255) NULL,
    [ShowMuddatInPymtReport] INTEGER NULL,
    [MuddatDays] DOUBLE NULL,
    [InterestRate] DOUBLE NULL,
    [LogoLeftMargin] DOUBLE NULL,
    [LogoTopMargin] DOUBLE NULL,
    [LogoHeight] DOUBLE NULL,
    [LogoWidth] DOUBLE NULL,
    [ShowSelfPurcExpSeperate] INTEGER NULL,
    [ItemWiseReportinPaymentPlan] INTEGER NULL,
    [KattaWeightPR3BCPA] DOUBLE NULL,
    [AmountTypeInAutoFillPartiesChqPrinting] INTEGER NULL,
    [RoundDigitsInAutoFillPartiesChqPrinting] INTEGER NULL,
    [ShowKachiAadhatOpeningStock] INTEGER NULL,
    [TempCol] INTEGER NULL,
    [ShowDalaliAdjustment] INTEGER NULL,
    [UseCreditLimitInLedger] INTEGER NULL,
    [CheckSrNoInCottonSale] INTEGER NULL,
    [OtherLedgerEnabledInChqPrinting] INTEGER NULL,
    [TDSDeductInList] INTEGER NULL,
    [WholesaleVchDiscountBeforeNetRate] INTEGER NULL,
    [BottomMarginMarketTypeBill] DOUBLE NULL,
    [DiscountPercentSale] DOUBLE NULL,
    [DiscountPercentPurc] DOUBLE NULL,
    [PrintAmountWithVATinWholesaleVch] INTEGER NULL,
    [ShowDiscAmountInBillPrintWholesale] INTEGER NULL,
    [AlwaysPrintHindiHeading] INTEGER NULL,
    [MillingWithAmountCrossChange] INTEGER NULL,
    [MillingWithAmountAutoPickItems] INTEGER NULL,
    [PymtMultiEntryMode] INTEGER NULL,
    [RcptMultiEntryMode] INTEGER NULL,
    [RcptAutoReceiptNo] INTEGER NULL,
    [UgraiReportGroupWise] INTEGER NULL,
    [InterestSeperatePostInMT] INTEGER NULL,
    [NonWovenLenWidthNotCalculate] INTEGER NULL,
    [AutoPickCustomerLastRateWholesaleVch] INTEGER NULL,
    [TDSRoundOff] INTEGER NULL,
    [ShowOtherPostingInWholesaleVch] INTEGER NULL,
    [OtherLedgerSeperatePostInChqPrinting] INTEGER NULL,
    [UseOtherExpAsPurcOldBattery] INTEGER NULL,
    [InterestTDSDeduct] INTEGER NULL,
    [ShowDateSummaryInMFeeReport] INTEGER NULL,
    [ShowDateSummaryInHRDFReport] INTEGER NULL,
    [InterestMuddatAfterTDS] INTEGER NULL,
    [DateItemTotalNotShowInMFeeReport] INTEGER NULL,
    [DateItemTotalNotShowInHRDFReport] INTEGER NULL,
    [AutoCalculationinStockItems] INTEGER NULL,
    [JrnlMultiEntryMode] INTEGER NULL,
    [DateItemTotalNotShowInMReport] INTEGER NULL,
    [DateItemTotalNotShowInHReport] INTEGER NULL,
    [AutoPickNextIteminWholesaleVch] INTEGER NULL,
    [PrintVATC4New] INTEGER NULL,
    [PrintInChallanMode] INTEGER NULL,
    [PrintonHalfPage] INTEGER NULL,
    [InterestTDSRound] INTEGER NULL,
    [ShowNetRateInBillPrintInWholesaleVch] INTEGER NULL,
    [SelfPurchaseShowInMReport] INTEGER NULL,
    [SelfPurchaseShowInAReport] INTEGER NULL,
    [AutoAdjustStockItemName] INTEGER NULL,
    [AndroidSysteminItemChoose] INTEGER NULL,
    [ShowVehNoInWholesaleVch] INTEGER NULL,
    [BagPacking] DOUBLE NULL,
    [LabourRate] VARCHAR(255) NULL,
    [TermCondition] TEXT NULL,
    [TermConditionFontHindi] INTEGER NULL,
    [BillTopMargin] DOUBLE NULL,
    [BillLeftMargin] DOUBLE NULL,
    [AutoPickAmount] INTEGER NULL,
    [GSTnotPick] INTEGER NULL,
    [CompanyId] INTEGER NULL,
    [MillingShowDateStock] INTEGER NULL,
    [CheckForPurcRate] INTEGER NULL,
    [TCSSettings] VARCHAR(255) NULL,
    [LedgerListAndroidSel] INTEGER NULL,
    [LedgerListShowClBal] INTEGER NULL,
    [GSTNUserName] VARCHAR(255) NULL,
    [GSTNPasswd] VARCHAR(255) NULL,
    [EWBUserName] VARCHAR(255) NULL,
    [EWBPasswd] VARCHAR(255) NULL,
    [YourID] VARCHAR(255) NULL,
    [EWayPrintFormat] VARCHAR(255) NULL,
    [CursorVerticalMove] INTEGER NULL,
    [LabourRt] VARCHAR(255) NULL,
    [tmpdttm] DATETIME NULL,
    [DigiSignAppPath] VARCHAR(255) NULL,
    [UseBillDate] INTEGER NULL,
    [OtherLabourType] VARCHAR(255) NULL,
    [DataSync] INTEGER NULL,
    [WtNote100Bales] INTEGER NULL,
    [CheckLotNoInCottonSale] INTEGER NULL,
    [UseOtherBroker] INTEGER NULL,
    [AutoRefreshTimberVch] INTEGER NULL,
    [SleepDelayInItemSelection] INTEGER NULL,
    [GSTDateCriteriaChoice] VARCHAR(255) NULL,
    [EInvDelay] INTEGER NULL,
    [GetStatusURL] VARCHAR(255) NULL,
    [DeleteURL] VARCHAR(255) NULL,
    [GetQRcodeURL] VARCHAR(255) NULL,
    [SendURL] VARCHAR(255) NULL,
    [AdditionalDetails] TEXT NULL
);

CREATE TABLE [PostDatedCheques] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [DrParty] INTEGER NULL,
    [ChequeDate] DATETIME NULL,
    [ChequeAmount] DOUBLE NULL,
    [ChequeNo] VARCHAR(255) NULL,
    [CrParty] INTEGER NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [PowderVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [AccountCode] INTEGER NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [Bags] DOUBLE NULL,
    [FactoryWeight] DOUBLE NULL,
    [PartyWeight] DOUBLE NULL,
    [SaudaRate] DOUBLE NULL,
    [LABName] VARCHAR(255) NULL,
    [Oil] DOUBLE NULL,
    [OilCut] DOUBLE NULL,
    [NetOil] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [OtherExp] DOUBLE NULL,
    [TaxType] VARCHAR(255) NULL,
    [TaxRate] DOUBLE NULL,
    [Tax] DOUBLE NULL,
    [NetAmount] DOUBLE NULL,
    [OtherLabResult] DOUBLE NULL,
    [OtherLabName] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [ReconcileVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] SMALLINT NULL,
    [TransType] VARCHAR(5) NULL,
    [EffectedDate] DATETIME NULL,
    [Narration] VARCHAR(50) NULL,
    [ReconcileDate] DATETIME NULL
);

CREATE TABLE [RiceBranVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [DrCr] VARCHAR(255) NULL,
    [AccountCode] INTEGER NULL,
    [Bags] DOUBLE NULL,
    [Qtl] DOUBLE NULL,
    [NetQtl] DOUBLE NULL,
    [Condition] VARCHAR(255) NULL,
    [Oil] DOUBLE NULL,
    [NetOil] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [Bardana] DOUBLE NULL,
    [Tax] DOUBLE NULL,
    [CD] DOUBLE NULL,
    [NetAmount] DOUBLE NULL,
    [Status] VARCHAR(255) NULL,
    [VehNo] VARCHAR(255) NULL,
    [FreightAccountCode] INTEGER NULL,
    [Weight] DOUBLE NULL,
    [FreightRate] DOUBLE NULL,
    [FreightAmount] DOUBLE NULL,
    [OtherCreditAmount] DOUBLE NULL,
    [OtherCreditAccountCode] INTEGER NULL,
    [OtherCreditRemarks] VARCHAR(255) NULL,
    [OtherDebitAmount] DOUBLE NULL,
    [OtherDebitAccountCode] INTEGER NULL,
    [OtherDebitRemarks] VARCHAR(255) NULL,
    [MarginAmount] DOUBLE NULL,
    [MarginAccountCode] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [RiceHuskVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [DrCr] VARCHAR(255) NULL,
    [AccountCode] INTEGER NULL,
    [VehNo] VARCHAR(255) NULL,
    [Weight] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [Others] DOUBLE NULL,
    [NetAmount] DOUBLE NULL,
    [FreightRate] DOUBLE NULL,
    [Freight] DOUBLE NULL,
    [FreightLedger] DOUBLE NULL,
    [OtherCreditAmount] DOUBLE NULL,
    [OtherCreditAccountCode] INTEGER NULL,
    [OtherCreditRemarks] VARCHAR(255) NULL,
    [OtherDebitAmount] DOUBLE NULL,
    [OtherDebitAccountCode] INTEGER NULL,
    [OtherDebitRemarks] VARCHAR(255) NULL,
    [MarginAmount] DOUBLE NULL,
    [MarginAccountCode] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL
);

CREATE TABLE [SaleExpHeading] (
    [RowNo] INTEGER NULL,
    [Heading] VARCHAR(255) NULL,
    [SaleMode] VARCHAR(255) NULL
);

CREATE TABLE [SaleTransportationDetail] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(5) NULL,
    [VehicleNo] VARCHAR(50) NULL,
    [GRNo] VARCHAR(50) NULL,
    [DriverName] VARCHAR(50) NULL,
    [ST38No] VARCHAR(50) NULL,
    [DispatchTime] VARCHAR(50) NULL,
    [DispatchDate] VARCHAR(50) NULL,
    [PurchaseOrderNo] VARCHAR(255) NULL,
    [BillItemName] VARCHAR(255) NULL,
    [Grade] VARCHAR(255) NULL,
    [ChallanNo] VARCHAR(255) NULL,
    [KandaWeight] DOUBLE NULL,
    [OtherInfo] VARCHAR(255) NULL,
    [BardanaType] VARCHAR(255) NULL,
    [BardanaBags] DOUBLE NULL,
    [BardanaRate] DOUBLE NULL,
    [BardanaAmount] DOUBLE NULL,
    [CalculateVATonFreight] INTEGER NULL,
    [CalculateCommissiononFreight] INTEGER NULL,
    [ShippingAddress] VARCHAR(255) NULL,
    [QRCode] BLOB NULL,
    [IRNNo] VARCHAR(255) NULL,
    [TransporterName] VARCHAR(255) NULL,
    [TransporterGSTIN] VARCHAR(255) NULL,
    [TransportMode] VARCHAR(255) NULL,
    [Distance] VARCHAR(255) NULL,
    [TransportDocNo] VARCHAR(255) NULL,
    [TransportDocDt] VARCHAR(255) NULL,
    [BuyerLocation] VARCHAR(255) NULL,
    [BuyerPINCode] VARCHAR(255) NULL,
    [EWayOthers] VARCHAR(255) NULL,
    [ShipFrom_GSTIN] VARCHAR(255) NULL,
    [ShipFrom_Nm] VARCHAR(255) NULL,
    [ShipFrom_Addr1] VARCHAR(255) NULL,
    [ShipFrom_Loc] VARCHAR(255) NULL,
    [ShipFrom_Pin] VARCHAR(255) NULL,
    [ShipFrom_Stcd] VARCHAR(255) NULL,
    [ShipFrom_Others] VARCHAR(255) NULL,
    [ShipTo_Gstin] VARCHAR(255) NULL,
    [ShipTo_Nm] VARCHAR(255) NULL,
    [ShipTo_Addr1] VARCHAR(255) NULL,
    [ShipTo_Loc] VARCHAR(255) NULL,
    [ShipTo_Pin] VARCHAR(255) NULL,
    [ShipTo_Stcd] VARCHAR(255) NULL,
    [ShipTo_Others] VARCHAR(255) NULL,
    [EInvTransType] VARCHAR(255) NULL,
    [EInvStatus] VARCHAR(255) NULL,
    [EWayStatus] VARCHAR(255) NULL,
    [ShippingBillDate] VARCHAR(255) NULL,
    [ShippingBillNo] VARCHAR(255) NULL,
    [ShippingPortCode] VARCHAR(255) NULL,
    [EInvPDF] TEXT NULL,
    [EInvAckNo] VARCHAR(255) NULL,
    [EInvAckDate] VARCHAR(255) NULL
);

CREATE TABLE [SaudaTransactions] (
    [RowNo] INTEGER NULL,
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [SaudaDate] DATETIME NULL,
    [SaudaVchNo] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [Qty] DOUBLE NULL,
    [Remarks] VARCHAR(255) NULL
);

CREATE TABLE [SaudaVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [AccountCode] INTEGER NULL,
    [ItemName] VARCHAR(255) NULL,
    [ItemCode] INTEGER NULL,
    [SaudaType] VARCHAR(255) NULL,
    [Weight] VARCHAR(255) NULL,
    [Rate] DOUBLE NULL,
    [Narration] VARCHAR(255) NULL,
    [ItemUnit] VARCHAR(255) NULL,
    [Broker] VARCHAR(255) NULL,
    [Condition] VARCHAR(255) NULL,
    [DueDt] DATETIME NULL,
    [SaudaNo] VARCHAR(255) NULL
);

CREATE TABLE [Settings] (
    [Less] REAL NULL,
    [LessLedger] SMALLINT NULL,
    [InsuranceCharges] REAL NULL,
    [InsuranceChargesLedger] SMALLINT NULL,
    [Jaffary] REAL NULL,
    [JaffaryLedger] SMALLINT NULL,
    [Brokerage] REAL NULL,
    [BrokerageLedger] SMALLINT NULL,
    [AuctionCharges] REAL NULL,
    [AssociationCharges] REAL NULL,
    [GaushalaCharges] REAL NULL,
    [CommissionOnSale] REAL NULL,
    [CommissionOnWhichTotal] VARCHAR(50) NULL,
    [TDSDami] REAL NULL,
    [DamiTDSDeductionAfter] DOUBLE NULL,
    [TDSFreight] REAL NULL,
    [FreightTDSDeductionAfter] DOUBLE NULL,
    [TDSCommission] REAL NULL,
    [CommissionTDSDeductionAfter] DOUBLE NULL,
    [TDSLabour] REAL NULL,
    [LabourTDSDeductionAfter] DOUBLE NULL,
    [TDSInterest] REAL NULL,
    [InterestTDSDeductionAfter] DOUBLE NULL,
    [TDSBrokerage] REAL NULL,
    [BrokerageTDSDeductionAfter] DOUBLE NULL,
    [TDSRent] REAL NULL,
    [RentTDSDeductionAfter] DOUBLE NULL,
    [AuctionChargesLedger] SMALLINT NULL,
    [AssociationChargesLedger] SMALLINT NULL,
    [GaushalaChargesLedger] SMALLINT NULL,
    [CommissionLedger] SMALLINT NULL,
    [OtherDamiRate] REAL NULL,
    [Bonus] REAL NULL,
    [BonusLedger] SMALLINT NULL,
    [Relief] REAL NULL,
    [ReliefLedger] SMALLINT NULL,
    [FreightTDSLedger] SMALLINT NULL,
    [InterestTDSLedger] SMALLINT NULL,
    [CommissionTDSLedger] SMALLINT NULL,
    [LabourTDSLedger] SMALLINT NULL,
    [DamiTDSLedger] SMALLINT NULL,
    [BrokerageTDSLedger] SMALLINT NULL,
    [RentTDSLedger] SMALLINT NULL,
    [MiscExpLedger] SMALLINT NULL,
    [SalaryLedger] SMALLINT NULL,
    [DepriciationLedger] SMALLINT NULL,
    [AdvanceLedger] SMALLINT NULL,
    [LabourLedger] SMALLINT NULL,
    [RoundOffLedger] SMALLINT NULL,
    [DiscountLedger] SMALLINT NULL,
    [FreightLedger] SMALLINT NULL,
    [InterestLedger] SMALLINT NULL,
    [OtherExpLedger] SMALLINT NULL,
    [JFormIFormLedger] SMALLINT NULL,
    [LabourUtraiLedger] SMALLINT NULL,
    [LabourJharaiLedger] SMALLINT NULL,
    [LabourBharaiLedger] SMALLINT NULL,
    [LabourTulaiLedger] SMALLINT NULL,
    [LabourKhichaiLedger] SMALLINT NULL,
    [LabourSilaiLedger] SMALLINT NULL,
    [LabourLoadingLedger] SMALLINT NULL,
    [JFrmLabourUtrai] BOOLEAN NOT NULL,
    [JFrmLabourJharai] BOOLEAN NOT NULL,
    [JFrmLabourBharai] BOOLEAN NOT NULL,
    [JFrmLabourTulai] BOOLEAN NOT NULL,
    [JFrmLabourKhichai] BOOLEAN NOT NULL,
    [JFrmLabourSilai] BOOLEAN NOT NULL,
    [JFrmLabourLoading] BOOLEAN NOT NULL,
    [IFrmLabourUtrai] BOOLEAN NOT NULL,
    [IFrmLabourJharai] BOOLEAN NOT NULL,
    [IFrmLabourBharai] BOOLEAN NOT NULL,
    [IFrmLabourTulai] BOOLEAN NOT NULL,
    [IFrmLabourKhichai] BOOLEAN NOT NULL,
    [IFrmLabourSilai] BOOLEAN NOT NULL,
    [IFrmLabourLoading] BOOLEAN NOT NULL,
    [PurchaseLabourUtrai] BOOLEAN NOT NULL,
    [PurchaseLabourJharai] BOOLEAN NOT NULL,
    [PurchaseLabourBharai] BOOLEAN NOT NULL,
    [PurchaseLabourTulai] BOOLEAN NOT NULL,
    [PurchaseLabourKhichai] BOOLEAN NOT NULL,
    [PurchaseLabourSilai] BOOLEAN NOT NULL,
    [PurchaseLabourLoading] BOOLEAN NOT NULL,
    [SaleLabourUtrai] BOOLEAN NOT NULL,
    [SaleLabourJharai] BOOLEAN NOT NULL,
    [SaleLabourBharai] BOOLEAN NOT NULL,
    [SaleLabourTulai] BOOLEAN NOT NULL,
    [SaleLabourKhichai] BOOLEAN NOT NULL,
    [SaleLabourSilai] BOOLEAN NOT NULL,
    [SaleLabourLoading] BOOLEAN NOT NULL,
    [DamiInSelfPurchase] BOOLEAN NOT NULL,
    [DamiInCommissionBasis] BOOLEAN NOT NULL,
    [ShowCashBalancesInPymtRcpt] DOUBLE NULL,
    [ShowLedgersBalanceInVoucherEntries] DOUBLE NULL,
    [TrialBalanceAlwaysExtract] DOUBLE NULL,
    [ShowNarrationAlwaysInLedgerVouchers] DOUBLE NULL,
    [ShowJFrmIFrmStockDiffIndicator] DOUBLE NULL,
    [PropPartnerName] TEXT NULL,
    [Place] TEXT NULL,
    [DatabaseUpdationDateTime] VARCHAR(255) NULL,
    [CommissionInStockTransfer] DOUBLE NULL,
    [FreightRatePerQtl] DOUBLE NULL,
    [ShowDheriesInSaleVoucher] DOUBLE NULL,
    [CheckDuplicateInvoiceNo] DOUBLE NULL,
    [AutoUseLastEnteredVoucherDate] DOUBLE NULL,
    [AutoDamiTDSDeductInPurchase] DOUBLE NULL,
    [CreditMarketFeeInPurchaseVoucher] DOUBLE NULL,
    [AuctionChargesInIForm] DOUBLE NULL,
    [ShowAccountFlatType] DOUBLE NULL,
    [PromptDateEveryTime] DOUBLE NULL,
    [PromptForPrintingEveryTime] DOUBLE NULL,
    [ShowLastEnteredVoucher] DOUBLE NULL,
    [PartyNameItemWiseInJFrm] INTEGER NULL,
    [AllowZeroAmountInPurchase] DOUBLE NULL,
    [ShowDamiTDSSeperate] DOUBLE NULL,
    [ShowDetailedJournal] DOUBLE NULL,
    [PrintByLaser] INTEGER NULL,
    [LedgerAutoShowByF12] INTEGER NULL,
    [BillContentsShowInLedgerVouchers] INTEGER NULL,
    [DoNotShowEntryHeadInLedgerVouchers] INTEGER NULL,
    [CheckDuplicateInvoiceNoInSale] DOUBLE NULL,
    [UseSpecialFormatForDMP] DOUBLE NULL,
    [ShowAdditionalItemsExtraInTrading] DOUBLE NULL,
    [Designation] VARCHAR(255) NULL,
    [ShowBagsWeightInJournal] DOUBLE NULL,
    [UseJointPL] INTEGER NULL,
    [PrintGroupNameInMFeeReport] INTEGER NULL,
    [PrintGroupNameInHRDFReport] INTEGER NULL,
    [AlertForSunday] INTEGER NULL,
    [DeductorName] VARCHAR(255) NULL,
    [DeductorType] VARCHAR(255) NULL,
    [DeductorBranch] VARCHAR(255) NULL,
    [DeductorAddress] VARCHAR(255) NULL,
    [DeductorCity] VARCHAR(255) NULL,
    [DeductorState] VARCHAR(255) NULL,
    [DeductorPINCode] VARCHAR(255) NULL,
    [DeductorPhoneNo] VARCHAR(255) NULL,
    [DeductorEmail] VARCHAR(255) NULL,
    [ResponsibleName] VARCHAR(255) NULL,
    [ResponsibleDesignation] VARCHAR(255) NULL,
    [ResponsibleAddress] VARCHAR(255) NULL,
    [ResponsibleCity] VARCHAR(255) NULL,
    [ResponsibleState] VARCHAR(255) NULL,
    [ResponsiblePINCode] VARCHAR(255) NULL,
    [ResponsiblePhoneNo] VARCHAR(255) NULL,
    [ResponsibleEmail] VARCHAR(255) NULL,
    [PromptForPrintingInChequePayment] INTEGER NULL,
    [PromptPostCheques] INTEGER NULL,
    [PromptPrintCheques] INTEGER NULL,
    [PromptPrintList] INTEGER NULL,
    [PromptSaveList] INTEGER NULL,
    [ShowQtyTotalInJournalBook] INTEGER NULL,
    [ShowQtyInPaymentReport] INTEGER NULL,
    [ShowStockSummaryDetailed] INTEGER NULL,
    [PrintItemNameInSaleTaxReports] INTEGER NULL,
    [PrintScheduleNoInSaleTaxReports] INTEGER NULL,
    [PrintFormsStockSummaryInTaxReport] INTEGER NULL,
    [PromptForPrintingIFrm] INTEGER NULL,
    [BankNameForMTSlips] VARCHAR(255) NULL,
    [PackingShowInLedgerAccount] INTEGER NULL,
    [ItemRowAmountShowInLedger] INTEGER NULL,
    [ShowVehNoInLedgerVouchers] INTEGER NULL,
    [ShowSaudaDateInLedgerVouchers] INTEGER NULL,
    [SurchargeLedger] INTEGER NULL,
    [AutoAdjustNarrationInTitleCase] INTEGER NULL,
    [DharmadaCalculateOn] VARCHAR(255) NULL,
    [PropSonof] VARCHAR(255) NULL,
    [CITAddress] VARCHAR(255) NULL,
    [CITCity] VARCHAR(255) NULL,
    [CITPinCode] VARCHAR(255) NULL,
    [DaysForAlertBackDateVch] INTEGER NULL,
    [PrintVoucher] INTEGER NULL,
    [VoucherSr] INTEGER NULL,
    [SGSTLedger] INTEGER NULL,
    [CGSTLedger] INTEGER NULL,
    [IGSTLedger] INTEGER NULL,
    [CESSLedger] INTEGER NULL,
    [ReverseChargeLedger] INTEGER NULL,
    [PromptForVATGST] INTEGER NULL,
    [SelfPINCode] VARCHAR(255) NULL,
    [CreditDebitNoteDetailsNotShowInLedger] INTEGER NULL,
    [AlwaysPromptForPrinterSelection] INTEGER NULL
);

CREATE TABLE [StockGodowns] (
    [GodownName] VARCHAR(255) NULL,
    [Code1st] INTEGER NULL,
    [Location] VARCHAR(255) NULL,
    [ItemCode] INTEGER NULL,
    [Bags] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [RowNo] INTEGER NULL,
    [Amount] DOUBLE NULL
);

CREATE TABLE [StockGodownTransactions] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [GodownFromCode] INTEGER NULL,
    [GodownToCode] INTEGER NULL,
    [ChallanNo] VARCHAR(255) NULL,
    [VehicleNo] VARCHAR(255) NULL,
    [GRNo] VARCHAR(255) NULL,
    [DispatchTime] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [ItemCode] INTEGER NULL,
    [Bags] DOUBLE NULL,
    [Packing] DOUBLE NULL,
    [Weight] DOUBLE NULL,
    [VATCST] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL
);

CREATE TABLE [StockGroups] (
    [GroupName] VARCHAR(50) NULL,
    [Code1st] SMALLINT NULL,
    [Code2nd] SMALLINT NULL,
    [Code3rd] SMALLINT NULL,
    [Code4th] SMALLINT NULL,
    [QtyNotShowInTrading] INTEGER NULL,
    [ClStockRate] DOUBLE NULL
);

CREATE TABLE [StockItems] (
    [ItemName] VARCHAR(50) NULL,
    [Code1st] SMALLINT NOT NULL,
    [GroupCode] SMALLINT NOT NULL,
    [UnitCode] SMALLINT NULL,
    [Packing] REAL NULL,
    [VAT] REAL NULL,
    [VATLedger] SMALLINT NULL,
    [CST] REAL NULL,
    [CSTLedger] SMALLINT NULL,
    [Dami] REAL NULL,
    [DamiLedger] SMALLINT NULL,
    [MarketFee] REAL NULL,
    [MarketFeeLedger] SMALLINT NULL,
    [HRDF] REAL NULL,
    [HRDFLedger] SMALLINT NULL,
    [PurchaseLedger] SMALLINT NULL,
    [SaleLedger] SMALLINT NULL,
    [StockLedger] SMALLINT NULL,
    [OpeningBags] DOUBLE NULL,
    [OpeningQty] DOUBLE NULL,
    [OpeningValue] DOUBLE NULL,
    [SaleRate] DOUBLE NULL,
    [OpeningBagsJFrmIFrm] DOUBLE NULL,
    [OpeningQtyJFrmIFrm] DOUBLE NULL,
    [OpeningValueJFrmIFrm] DOUBLE NULL,
    [LabourRateUnit] VARCHAR(10) NULL,
    [WeightKgFrom1] REAL NULL,
    [WeightKgTo1] REAL NULL,
    [UtraiRate1] REAL NULL,
    [JharaiRate1] REAL NULL,
    [BharaiRate1] REAL NULL,
    [TulaiRate1] REAL NULL,
    [KhichaiRate1] REAL NULL,
    [SilaiRate1] REAL NULL,
    [LoadingRate1] REAL NULL,
    [WeightKgFrom2] REAL NULL,
    [WeightKgTo2] REAL NULL,
    [UtraiRate2] REAL NULL,
    [JharaiRate2] REAL NULL,
    [BharaiRate2] REAL NULL,
    [TulaiRate2] REAL NULL,
    [KhichaiRate2] REAL NULL,
    [SilaiRate2] REAL NULL,
    [LoadingRate2] REAL NULL,
    [WeightKgFrom3] REAL NULL,
    [WeightKgTo3] REAL NULL,
    [UtraiRate3] REAL NULL,
    [JharaiRate3] REAL NULL,
    [BharaiRate3] REAL NULL,
    [TulaiRate3] REAL NULL,
    [KhichaiRate3] REAL NULL,
    [SilaiRate3] REAL NULL,
    [LoadingRate3] REAL NULL,
    [BagsSelfPurchase] DOUBLE NULL,
    [CurrentBalance] DOUBLE NULL,
    [SelfTradingValue] DOUBLE NULL,
    [BagsJFrmIFrm] DOUBLE NULL,
    [CurrentBalanceJFrmIFrm] DOUBLE NULL,
    [JFrmIFrmValue] DOUBLE NULL,
    [ItemType] VARCHAR(12) NULL,
    [LabourChargeFromParty1] DOUBLE NULL,
    [LabourChargeFromParty2] DOUBLE NULL,
    [LabourChargeFromParty3] DOUBLE NULL,
    [PurchaseReturnLedger] DOUBLE NULL,
    [SaleReturnLedger] DOUBLE NULL,
    [CalculateInTrading] INTEGER NULL,
    [DDInsentive] DOUBLE NULL,
    [VATLowerRate] DOUBLE NULL,
    [CSTWithoutCForm] DOUBLE NULL,
    [PurcRate] DOUBLE NULL,
    [AutoSplitUpdate] INTEGER NULL,
    [AutoCalculateAmountInSale] INTEGER NULL,
    [CapitalGoods] INTEGER NULL,
    [MRPGoods] INTEGER NULL,
    [ScheduleNo] VARCHAR(255) NULL,
    [ItemNarration] VARCHAR(255) NULL,
    [TaxPayable] INTEGER NULL,
    [StockCalculate] INTEGER NULL,
    [MRP] DOUBLE NULL,
    [MillingItem] INTEGER NULL,
    [ItemTypeForManufacturing] VARCHAR(255) NULL,
    [EmptyRequire] VARCHAR(255) NULL,
    [Surcharge] DOUBLE NULL,
    [OneTimeTaxPaid] INTEGER NULL,
    [VATonCommission] DOUBLE NULL,
    [CompanyName] VARCHAR(255) NULL,
    [CommRate] DOUBLE NULL,
    [MarketCommttFormApply] INTEGER NULL,
    [AutoClStockUnit] VARCHAR(255) NULL,
    [AutoClStockUnitRate] DOUBLE NULL,
    [MarketCommttCouponApply] INTEGER NULL,
    [OpClCalculate] INTEGER NULL,
    [Discountt] DOUBLE NULL,
    [ServiceTaxRate] DOUBLE NULL,
    [ItemCategory] VARCHAR(255) NULL,
    [RateCalcOn] VARCHAR(255) NULL,
    [GSTRateSlab] VARCHAR(255) NULL,
    [HSNCode] VARCHAR(255) NULL,
    [ReverseChargeApply] INTEGER NULL,
    [CessCalcOnQty] INTEGER NULL,
    [SAPCode] VARCHAR(255) NULL,
    [Digits] INTEGER NULL,
    [GoodsType] VARCHAR(255) NULL,
    [KrishiKalyanKosh] DOUBLE NULL,
    [MFeeCalcOnWeight] INTEGER NULL,
    [DamiCalcOnWeight] INTEGER NULL,
    [MFeeAfterAmount] DOUBLE NULL,
    [MFeeAfterRate] DOUBLE NULL,
    [HRDFAfterAmount] DOUBLE NULL,
    [HRDFAfterRate] DOUBLE NULL,
    [CessCalcOnMRP] INTEGER NULL,
    [CCessCalcOnAmt] INTEGER NULL
);

CREATE TABLE [StockTransactions] (
    [RowNo] SMALLINT NULL,
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(5) NULL,
    [ItemCode] SMALLINT NULL,
    [Bags] DOUBLE NULL,
    [Packing] REAL NULL,
    [Weight] DOUBLE NULL,
    [VATCST] REAL NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [DheriPurchaseDate] DATETIME NULL,
    [DheriPurchaseFrom] SMALLINT NULL,
    [DheriBillNo] VARCHAR(50) NULL,
    [DheriNo] SMALLINT NULL,
    [TaxableAmount] DOUBLE NULL,
    [Tax] DOUBLE NULL,
    [TaxType] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [VoucherType] VARCHAR(255) NULL,
    [CommissionPartyCode] INTEGER NULL,
    [GodownCode] INTEGER NULL,
    [ItemScheme] DOUBLE NULL,
    [ItemMRP] DOUBLE NULL,
    [ItemDiscount] DOUBLE NULL,
    [SurchargeRate] DOUBLE NULL,
    [SurchargeAmount] DOUBLE NULL,
    [CashScheme] DOUBLE NULL,
    [ItemDiscount2] DOUBLE NULL,
    [TimberItemRowNo] INTEGER NULL,
    [LooseWeight] DOUBLE NULL,
    [TaxIncluding] VARCHAR(255) NULL,
    [MarketFeeRate] REAL NULL,
    [HRDFRate] REAL NULL,
    [HideBags] DOUBLE NULL,
    [ActivationDate] DATETIME NULL,
    [DiscountBeforeNetRate] DOUBLE NULL,
    [MktCommttCoupNo] VARCHAR(255) NULL,
    [SGSTTax] DOUBLE NULL,
    [CGSTTax] DOUBLE NULL,
    [IGSTTax] DOUBLE NULL,
    [CESSTax] DOUBLE NULL,
    [ReverseCharge] INTEGER NULL,
    [DiscBeforeCashSch] REAL NULL,
    [CessAmt] DOUBLE NULL,
    [CCessAmt] DOUBLE NULL,
    [CCessRate] DOUBLE NULL,
    [CessCalconQtyStatus] INTEGER NULL,
    [CessCalconMRPStatus] INTEGER NULL,
    [CCessOnAmountStatus] INTEGER NULL,
    [IFormBags] DOUBLE NULL
);

CREATE TABLE [StockTypes] (
    [StockTypes] VARCHAR(50) NULL
);

CREATE TABLE [StockUnits] (
    [UnitName] VARCHAR(50) NULL,
    [Code1st] SMALLINT NOT NULL,
    [DecimalPlaces] SMALLINT NULL,
    [RotationNumber] SMALLINT NULL
);

CREATE TABLE [TaxDeposits] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [TransType] VARCHAR(255) NULL,
    [IncomeFromDate] DATETIME NULL,
    [IncomeToDate] DATETIME NULL,
    [Amount] DOUBLE NULL,
    [ChallanNo] VARCHAR(255) NULL,
    [BankBranch] VARCHAR(255) NULL,
    [TaxType] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [BasicTax] VARCHAR(255) NULL,
    [Interest] VARCHAR(255) NULL,
    [Penalty] VARCHAR(255) NULL,
    [Others] VARCHAR(255) NULL,
    [DepartmentDemandTax] VARCHAR(255) NULL,
    [PostInBooks] INTEGER NULL,
    [BankBranchName] VARCHAR(255) NULL
);

CREATE TABLE [tblWhatsapp] (
    [id] INTEGER NOT NULL,
    [instanceID] TEXT NULL,
    [access_tocken] TEXT NULL,
    [SenderKey] VARCHAR(255) NULL,
    [SenderMobNo] VARCHAR(255) NULL
);

CREATE TABLE [TDSAcknowLedgementNos] (
    [RowNo] INTEGER NULL,
    [QuarterFromDate] DATETIME NULL,
    [QuarterToDate] DATETIME NULL,
    [AcknowLedgementNo] VARCHAR(255) NULL,
    [IsTDSTypeSalary] VARCHAR(255) NULL,
    [TDSType] VARCHAR(255) NULL
);

CREATE TABLE [TDSDeductions] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [TransType] VARCHAR(255) NULL,
    [AccountCode] DOUBLE NULL,
    [IncomeAmount] DOUBLE NULL,
    [TDS] DOUBLE NULL,
    [Surcharge] DOUBLE NULL,
    [Education] DOUBLE NULL,
    [TDSRate] DOUBLE NULL,
    [IncomeType] VARCHAR(255) NULL,
    [ReasonForNonDeduction] VARCHAR(255) NULL,
    [RateTDS] DOUBLE NULL,
    [RateSurcharge] DOUBLE NULL,
    [RateEducation] DOUBLE NULL,
    [PostInBooks] INTEGER NULL,
    [ChallanVoucherDate] DATETIME NULL,
    [ChallanVoucherNumber] INTEGER NULL,
    [IncomeNarration] VARCHAR(255) NULL,
    [PrevIncomeAmount] DOUBLE NULL
);

CREATE TABLE [TDSDeposits] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] DOUBLE NULL,
    [TransType] VARCHAR(255) NULL,
    [AccountCode] DOUBLE NULL,
    [IncomeFromDate] DATETIME NULL,
    [IncomeToDate] DATETIME NULL,
    [Amount] DOUBLE NULL,
    [ChallanNo] VARCHAR(255) NULL,
    [BankBranch] VARCHAR(255) NULL,
    [TDSType] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [ExtraAmountAdjustIn] VARCHAR(255) NULL,
    [Interest] VARCHAR(255) NULL,
    [Penalty] VARCHAR(255) NULL,
    [Others] VARCHAR(255) NULL,
    [PostInBooks] INTEGER NULL,
    [BooksVoucherDate] DATETIME NULL,
    [BooksVoucherNumber] INTEGER NULL,
    [BankBranchName] VARCHAR(255) NULL,
    [TDSTCS] VARCHAR(255) NULL,
    [TDSRt] VARCHAR(255) NULL
);

CREATE TABLE [TDSRates] (
    [RowNo] INTEGER NULL,
    [TotalTax] DOUBLE NULL,
    [TDS] DOUBLE NULL,
    [Surcharge] DOUBLE NULL,
    [Education] DOUBLE NULL,
    [AfterAmount] DOUBLE NULL,
    [IncomeType] VARCHAR(255) NULL,
    [FromDate] DATETIME NULL,
    [ToDate] DATETIME NULL
);

CREATE TABLE [TDSSettings] (
    [TDSRoundOff] BOOLEAN NOT NULL,
    [RefreshReport] BOOLEAN NOT NULL,
    [ReportType] VARCHAR(50) NULL
);

CREATE TABLE [TempAankTypeInterest] (
    [RowNo] INTEGER NULL,
    [PartyName] VARCHAR(255) NULL,
    [CrAank] VARCHAR(255) NULL,
    [CrAmount] VARCHAR(255) NULL,
    [CrDate] VARCHAR(255) NULL,
    [CrDays] INTEGER NULL,
    [CrDiscDays] INTEGER NULL,
    [CrDiscAank] VARCHAR(255) NULL,
    [CrBold] INTEGER NULL,
    [DrAank] VARCHAR(255) NULL,
    [DrAmount] VARCHAR(255) NULL,
    [DrDate] VARCHAR(255) NULL,
    [DrDays] INTEGER NULL,
    [DrDiscDays] INTEGER NULL,
    [DrDiscAank] VARCHAR(255) NULL,
    [DrBold] INTEGER NULL,
    [CrTotalAmount] VARCHAR(255) NULL,
    [DrTotalAmount] VARCHAR(255) NULL,
    [CrTotalAank] VARCHAR(255) NULL,
    [DrTotalAank] VARCHAR(255) NULL,
    [CrTotalDiscAank] VARCHAR(255) NULL,
    [DrTotalDiscAank] VARCHAR(255) NULL,
    [CrTotalInterestableAank] VARCHAR(255) NULL,
    [DrTotalInterestableAank] VARCHAR(255) NULL,
    [CrInterest] VARCHAR(255) NULL,
    [DrInterest] VARCHAR(255) NULL,
    [BalanceAmountString] VARCHAR(255) NULL,
    [BalanceAmount] VARCHAR(255) NULL,
    [InterestAmountString] VARCHAR(255) NULL,
    [InterestAmount] VARCHAR(255) NULL,
    [NetAmountString] VARCHAR(255) NULL,
    [NetAmount] VARCHAR(255) NULL
);

CREATE TABLE [TempBalanceSheet] (
    [RowNo] INTEGER NULL,
    [CrLedger] VARCHAR(255) NULL,
    [CrAmount] DOUBLE NULL,
    [DrLedger] VARCHAR(255) NULL,
    [DrAmount] DOUBLE NULL
);

CREATE TABLE [TempBalanceSheetFigures] (
    [RowNo] INTEGER NOT NULL,
    [LedgerName] VARCHAR(255) NULL,
    [DrCr] VARCHAR(255) NULL,
    [Amount] DOUBLE NULL
);

CREATE TABLE [TempCancelEInvoiceTable] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL
);

CREATE TABLE [TempCancelEWayTable] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL
);

CREATE TABLE [TempCashBook] (
    [VoucherDate] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [LedgerName] VARCHAR(255) NULL,
    [TransType] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [DrAmount] DOUBLE NULL,
    [CrAmount] DOUBLE NULL,
    [RunningBal] DOUBLE NULL,
    [TransRow] INTEGER NULL,
    [DateFirstRecord] INTEGER NULL,
    [PrintFontName] VARCHAR(255) NULL
);

CREATE TABLE [TempCheckGridTax] (
    [RowNo] INTEGER NULL,
    [TaxableAmount] DOUBLE NULL,
    [TaxRate] DOUBLE NULL,
    [TotalTax] DOUBLE NULL
);

CREATE TABLE [TempChqPymtNarration] (
    [AccountName] VARCHAR(50) NOT NULL,
    [DocumentNo] VARCHAR(25) NOT NULL,
    [LastEntryDate] VARCHAR(15) NOT NULL
);

CREATE TABLE [TempCommissionBasisDheriWiseStock] (
    [PartyCode] SMALLINT NULL,
    [PurchaseFromLedger] SMALLINT NULL,
    [ItemCode] SMALLINT NULL,
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [Bags] DOUBLE NULL,
    [Packing] REAL NULL,
    [Weight] DOUBLE NULL,
    [VAT] REAL NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [TaxStatus] VARCHAR(20) NULL,
    [InvoiceNo] VARCHAR(50) NULL,
    [RowNo] SMALLINT NULL,
    [Narration] VARCHAR(255) NULL,
    [TransType] VARCHAR(255) NULL
);

CREATE TABLE [TempCompoundInterestDates] (
    [RowNo] INTEGER NULL,
    [InterestDate] DATETIME NULL
);

CREATE TABLE [TempDistance] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL
);

CREATE TABLE [TempDrCrNoteHSNReport] (
    [RowNo] INTEGER NULL,
    [NoteType] VARCHAR(255) NULL,
    [NoteMode] VARCHAR(255) NULL,
    [HSNCode] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [ItemCode] INTEGER NULL,
    [UnitName] VARCHAR(255) NULL,
    [TaxRate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [CGST] DOUBLE NULL,
    [SGST] DOUBLE NULL,
    [IGST] DOUBLE NULL,
    [CESS] DOUBLE NULL,
    [ItemQty] DOUBLE NULL
);

CREATE TABLE [tempEinvoice] (
    [SheetName] VARCHAR(255) NULL,
    [Header_row] INTEGER NULL,
    [StartingRow] INTEGER NULL,
    [RowNo] INTEGER NULL,
    [Field_Name] VARCHAR(255) NULL,
    [Field_Value] VARCHAR(255) NULL
);

CREATE TABLE [TempEInvoiceTable] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL,
    [FieldHeading_Jsn] VARCHAR(255) NULL,
    [FieldNm_Jsn] VARCHAR(255) NULL,
    [FieldType_Jsn] VARCHAR(255) NULL
);

CREATE TABLE [TempEWayTableByIRN] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL
);

CREATE TABLE [TempFlatTypeInterest] (
    [RowNo] INTEGER NULL,
    [PartyName] VARCHAR(255) NULL,
    [CrDate] VARCHAR(255) NULL,
    [CrAmount] DOUBLE NULL,
    [CrRate] REAL NULL,
    [CrD] INTEGER NULL,
    [CrDays] INTEGER NULL,
    [CrInterest] DOUBLE NULL,
    [DrDate] VARCHAR(255) NULL,
    [DrAmount] DOUBLE NULL,
    [DrRate] REAL NULL,
    [DrD] INTEGER NULL,
    [DrDays] INTEGER NULL,
    [DrInterest] DOUBLE NULL,
    [ResultString] VARCHAR(255) NULL,
    [NetBalance] VARCHAR(255) NULL,
    [ClBal] VARCHAR(255) NULL,
    [Intt] VARCHAR(255) NULL,
    [NarrationCr] VARCHAR(255) NULL,
    [NarrationDr] VARCHAR(255) NULL
);

CREATE TABLE [TempForm16A] (
    [RowNo] INTEGER NULL,
    [PartyName] VARCHAR(255) NULL,
    [Address] VARCHAR(255) NULL,
    [PAN] VARCHAR(255) NULL,
    [IncomeAmount] DOUBLE NULL,
    [PaymentDate] VARCHAR(255) NULL,
    [TDS] DOUBLE NULL,
    [Surcharge] DOUBLE NULL,
    [Education] DOUBLE NULL,
    [TotalTax] DOUBLE NULL,
    [ChqDDNo] VARCHAR(255) NULL,
    [BranchCode] VARCHAR(255) NULL,
    [TaxDepositDate] VARCHAR(255) NULL,
    [ChallanNo] VARCHAR(255) NULL,
    [AmountInWords] VARCHAR(255) NULL,
    [PartyPrefix] VARCHAR(255) NULL,
    [PartyPrefix1] VARCHAR(255) NULL,
    [AmountInWords1] VARCHAR(255) NULL,
    [AmountInWords2] VARCHAR(255) NULL
);

CREATE TABLE [TempGroupedBalanceSheet] (
    [RowNo] INTEGER NULL,
    [Item1] VARCHAR(255) NULL,
    [Item1Bold] INTEGER NULL,
    [Item2] VARCHAR(255) NULL,
    [Item3] VARCHAR(255) NULL,
    [Item3Bold] INTEGER NULL,
    [Item4] VARCHAR(255) NULL,
    [Item1NotUL] INTEGER NULL,
    [Item3NotUL] INTEGER NULL
);

CREATE TABLE [TempGSTNStatus] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL
);

CREATE TABLE [TempGSTR1B2CS] (
    [RowNo] INTEGER NOT NULL,
    [OEType] VARCHAR(255) NULL,
    [POS] VARCHAR(255) NULL,
    [TaxRate] DOUBLE NULL,
    [TaxableValue] DOUBLE NULL,
    [Cess] DOUBLE NULL,
    [ECommGSTIN] VARCHAR(255) NULL
);

CREATE TABLE [tempGSTR2A] (
    [RowNo] INTEGER NOT NULL,
    [PartyGSTIN] VARCHAR(255) NULL,
    [PartyName] VARCHAR(255) NULL,
    [DocType] VARCHAR(255) NULL,
    [DocType1] VARCHAR(255) NULL,
    [DocNo] VARCHAR(255) NULL,
    [DocDate] DATETIME NULL,
    [Taxable] DOUBLE NULL,
    [IGST] DOUBLE NULL,
    [CGST] DOUBLE NULL,
    [SGST] DOUBLE NULL,
    [CESS] DOUBLE NULL
);

CREATE TABLE [TempHSNReport] (
    [RowNo] INTEGER NULL,
    [HSN] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [UnitName] VARCHAR(255) NULL,
    [Qty] DOUBLE NULL,
    [TotalAmount] DOUBLE NULL,
    [TaxableAmount] DOUBLE NULL,
    [IGST] DOUBLE NULL,
    [CGST] DOUBLE NULL,
    [SGST] DOUBLE NULL,
    [CESS] DOUBLE NULL,
    [TaxRate] DOUBLE NULL
);

CREATE TABLE [TempIFrmTDSReport] (
    [PartyCode] INTEGER NULL,
    [VoucherDate] DATETIME NULL,
    [TransType] VARCHAR(255) NULL,
    [Amount] DOUBLE NULL,
    [TDS] DOUBLE NULL,
    [VoucherNumber] INTEGER NULL
);

CREATE TABLE [TempItemStockDetails] (
    [ItemCode] INTEGER NULL,
    [Pcs] DOUBLE NULL,
    [Qty] DOUBLE NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [BatchNo] VARCHAR(255) NULL,
    [ExpiryDate] DATETIME NULL,
    [PurcDate] DATETIME NULL,
    [RowNo] INTEGER NULL
);

CREATE TABLE [TempJFrmIFrmPartyStock] (
    [VoucherDate] DATETIME NULL,
    [PartyCode] SMALLINT NULL,
    [ItemCode] DOUBLE NULL,
    [Bags] DOUBLE NULL,
    [Packing] REAL NULL,
    [Weight] DOUBLE NULL,
    [VAT] REAL NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [LooseWeight] DOUBLE NULL
);

CREATE TABLE [TempJrnlBook] (
    [VoucherDate] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [CrName] VARCHAR(255) NULL,
    [CrAmount] DOUBLE NULL,
    [CrHead] INTEGER NULL,
    [DrName] VARCHAR(255) NULL,
    [DrAmount] DOUBLE NULL,
    [DrHead] INTEGER NULL,
    [DateFirstRecord] INTEGER NULL,
    [TitleLang] VARCHAR(255) NULL,
    [TitleText] VARCHAR(255) NULL
);

CREATE TABLE [TempLastEnteredStockItem] (
    [ItemCode] SMALLINT NULL,
    [Rate] DOUBLE NULL,
    [VoucherType] VARCHAR(5) NULL,
    [SubVoucherType] VARCHAR(50) NULL,
    [IAmLastVoucher] BOOLEAN NOT NULL
);

CREATE TABLE [TempLastEnteredVoucher] (
    [VoucherType] VARCHAR(5) NULL,
    [VoucherDate] DATETIME NULL,
    [AccountingYearFrom] DATETIME NULL,
    [AccountingYearTo] DATETIME NULL
);

CREATE TABLE [TempLedgerBook] (
    [LedgerName] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [CrDate] VARCHAR(255) NULL,
    [CrLedger] VARCHAR(255) NULL,
    [CrHead] INTEGER NULL,
    [CrAmount] DOUBLE NULL,
    [DrDate] VARCHAR(255) NULL,
    [DrLedger] VARCHAR(255) NULL,
    [DrHead] INTEGER NULL,
    [DrAmount] DOUBLE NULL,
    [TIN] VARCHAR(255) NULL,
    [PAN] VARCHAR(255) NULL,
    [LedgerFirstRecord] INTEGER NULL,
    [ClosingBal] VARCHAR(255) NULL,
    [CrQtyTotal] VARCHAR(255) NULL,
    [DrQtyTotal] VARCHAR(255) NULL
);

CREATE TABLE [TempLedgerBook1] (
    [RowNo] INTEGER NULL,
    [VoucherDate] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [Narration1] VARCHAR(255) NULL,
    [Debits] DOUBLE NULL,
    [Credits] DOUBLE NULL,
    [Balance] VARCHAR(255) NULL,
    [LedgerName] VARCHAR(255) NULL,
    [BagsWeightString] VARCHAR(255) NULL,
    [TINPAN] VARCHAR(255) NULL,
    [Address] VARCHAR(255) NULL,
    [Narration2] VARCHAR(255) NULL,
    [LedgerFirstRecord] INTEGER NULL,
    [Narration3] VARCHAR(255) NULL,
    [Narration4] VARCHAR(255) NULL,
    [Narration5] VARCHAR(255) NULL,
    [Narration6] VARCHAR(255) NULL,
    [Narration7] VARCHAR(255) NULL,
    [Narration8] VARCHAR(255) NULL,
    [Narration9] VARCHAR(255) NULL,
    [Narration10] VARCHAR(255) NULL
);

CREATE TABLE [TempMandiTypeInterest] (
    [RowNo] INTEGER NULL,
    [PartyName] VARCHAR(255) NULL,
    [DrDate] VARCHAR(255) NULL,
    [DrAmount] DOUBLE NULL,
    [CrDate] VARCHAR(255) NULL,
    [CrAmount] DOUBLE NULL,
    [InterestAmount] DOUBLE NULL,
    [ActualDays] INTEGER NULL,
    [DaysAfterDue] INTEGER NULL,
    [DrInterest] DOUBLE NULL,
    [CrInterest] DOUBLE NULL,
    [ResultString] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [NetBalance] VARCHAR(255) NULL,
    [ClBal] VARCHAR(255) NULL,
    [Intt] VARCHAR(255) NULL
);

CREATE TABLE [TempMFeeHRDF] (
    [Item1] VARCHAR(255) NULL,
    [Item2] VARCHAR(255) NULL,
    [Item3] VARCHAR(255) NULL,
    [Item4] VARCHAR(255) NULL,
    [Item5] VARCHAR(255) NULL,
    [Item6] VARCHAR(255) NULL,
    [Item7] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [Item8] VARCHAR(255) NULL,
    [DamiRate] DOUBLE NULL,
    [AuctionRate] DOUBLE NULL,
    [LabourRate] DOUBLE NULL,
    [Item9] VARCHAR(255) NULL
);

CREATE TABLE [TempMTInterestTDSReport] (
    [PartyCode] INTEGER NULL,
    [VoucherDate] DATETIME NULL,
    [TransType] VARCHAR(255) NULL,
    [Amount] DOUBLE NULL,
    [TDS] DOUBLE NULL,
    [VoucherNumber] INTEGER NULL
);

CREATE TABLE [TempOtherTable] (
    [RowNo] INTEGER NULL,
    [TempQRCode] BLOB NULL
);

CREATE TABLE [TempPartiesInterestList] (
    [PartyName] VARCHAR(255) NULL,
    [DrAmount] VARCHAR(255) NULL,
    [CrAmount] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [PANNo] VARCHAR(255) NULL,
    [VoucherDate] VARCHAR(255) NULL,
    [SrNo] INTEGER NULL,
    [BillDetail] INTEGER NULL
);

CREATE TABLE [TempPrintCheques] (
    [VoucherDate] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [PartyName] VARCHAR(255) NULL,
    [AmountFigures] VARCHAR(255) NULL,
    [AmountWords] VARCHAR(255) NULL,
    [BankCode] INTEGER NULL,
    [PartyShopNo] DOUBLE NULL
);

CREATE TABLE [TempPrintEInvoiceTable] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL
);

CREATE TABLE [TempPrintEWayTable] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL
);

CREATE TABLE [TempPrintMilkParchi] (
    [VoucherDate] DATETIME NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [PatientName] VARCHAR(255) NULL,
    [TestName] VARCHAR(255) NULL,
    [DoctorName] VARCHAR(255) NULL,
    [Report] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [txtAlign] INTEGER NULL
);

CREATE TABLE [TempPrintSaleRegister] (
    [RowNo] INTEGER NULL,
    [SubRowNo] INTEGER NULL,
    [Text1] VARCHAR(255) NULL,
    [Text2] VARCHAR(255) NULL,
    [Text3] VARCHAR(255) NULL,
    [Text4] VARCHAR(255) NULL,
    [Text5] VARCHAR(255) NULL,
    [Text6] VARCHAR(255) NULL,
    [Text7] VARCHAR(255) NULL,
    [Text8] VARCHAR(255) NULL,
    [Text9] VARCHAR(255) NULL,
    [Text10] VARCHAR(255) NULL,
    [Text11] VARCHAR(255) NULL,
    [Text12] VARCHAR(255) NULL,
    [Text13] VARCHAR(255) NULL,
    [Text14] VARCHAR(255) NULL,
    [Text15] VARCHAR(255) NULL,
    [Text16] VARCHAR(255) NULL,
    [Text17] VARCHAR(255) NULL,
    [Text18] VARCHAR(255) NULL,
    [Text19] VARCHAR(255) NULL,
    [Text20] VARCHAR(255) NULL,
    [Bags] VARCHAR(255) NULL,
    [Weight] VARCHAR(255) NULL,
    [VehNo] VARCHAR(255) NULL,
    [GRNo] VARCHAR(255) NULL,
    [ST38No] VARCHAR(255) NULL,
    [PurchaseOrderNo] VARCHAR(255) NULL,
    [Grade] VARCHAR(255) NULL,
    [Narration] VARCHAR(255) NULL,
    [GroupString] VARCHAR(255) NULL,
    [EntryDate] DATETIME NULL,
    [TimberSrNo] INTEGER NULL
);

CREATE TABLE [TempPrintTradingAccount] (
    [RowNo] INTEGER NULL,
    [RunningGroup] VARCHAR(255) NULL,
    [Item1] VARCHAR(255) NULL,
    [Item2] VARCHAR(255) NULL,
    [Item3] VARCHAR(255) NULL,
    [Item4] VARCHAR(255) NULL,
    [Item5] VARCHAR(255) NULL,
    [Item6] VARCHAR(255) NULL,
    [LastItem] INTEGER NULL,
    [UnitName] VARCHAR(255) NULL,
    [BoldUL1] INTEGER NULL,
    [FontShort1] INTEGER NULL,
    [BoldUL2] INTEGER NULL,
    [FontShort2] INTEGER NULL
);

CREATE TABLE [TempPrintVch] (
    [R_R] INTEGER NULL,
    [R_T] VARCHAR(255) NULL,
    [R_D] DATETIME NULL,
    [R_D1] DATETIME NULL
);

CREATE TABLE [TempPrintVouchers] (
    [RowNo] INTEGER NOT NULL,
    [VoucherDate] DATETIME NULL,
    [TransType] VARCHAR(255) NULL,
    [VoucherNumber] INTEGER NULL
);

CREATE TABLE [TempPurcLabourTDSReport] (
    [PartyCode] INTEGER NULL,
    [VoucherDate] DATETIME NULL,
    [TransType] VARCHAR(255) NULL,
    [Amount] DOUBLE NULL,
    [TDS] DOUBLE NULL,
    [VoucherNumber] INTEGER NULL
);

CREATE TABLE [TempQuarterlyReturnBillWise] (
    [RowNo] DOUBLE NULL,
    [SrNo] VARCHAR(255) NULL,
    [PartyName] VARCHAR(255) NULL,
    [TIN] VARCHAR(255) NULL,
    [BillNo] VARCHAR(255) NULL,
    [DATED] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [TaxableAmount] DOUBLE NULL,
    [Tax] DOUBLE NULL,
    [FormNo] VARCHAR(255) NULL
);

CREATE TABLE [TempQuarterlyReturnPartyWise] (
    [RowNo] DOUBLE NULL,
    [SrNo] VARCHAR(255) NULL,
    [PartyName] VARCHAR(255) NULL,
    [TIN] VARCHAR(255) NULL,
    [Figure1] DOUBLE NULL,
    [Figure2] DOUBLE NULL,
    [Figure3] DOUBLE NULL,
    [Figure4] DOUBLE NULL
);

CREATE TABLE [TempRcptPymtDetail] (
    [RowNo] INTEGER NULL,
    [LedgerName] VARCHAR(255) NULL,
    [OpAmount] VARCHAR(255) NULL,
    [Receipts] VARCHAR(255) NULL,
    [Payments] VARCHAR(255) NULL,
    [ClAmount] VARCHAR(255) NULL
);

CREATE TABLE [TemprptVoucherEntries] (
    [RowNo] SMALLINT NULL,
    [SubRowNo] SMALLINT NULL,
    [VoucherDate] VARCHAR(15) NULL,
    [LedgerName] VARCHAR(50) NULL,
    [AsPerDetails] VARCHAR(50) NULL,
    [DetailsDrCr] VARCHAR(50) NULL,
    [DetailsNarration] VARCHAR(250) NULL,
    [TransType] VARCHAR(5) NULL,
    [VoucherNumber] INTEGER NULL,
    [DrAmount] DOUBLE NULL,
    [CrAmount] DOUBLE NULL,
    [Narration] VARCHAR(100) NULL,
    [VchDate] DATETIME NULL
);

CREATE TABLE [TempSelfPurchaseDheriWiseStock] (
    [PurchaseFromLedger] SMALLINT NULL,
    [ItemCode] SMALLINT NULL,
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [Bags] DOUBLE NULL,
    [Packing] REAL NULL,
    [Weight] DOUBLE NULL,
    [VAT] REAL NULL,
    [Rate] DOUBLE NULL,
    [Amount] DOUBLE NULL,
    [TaxStatus] VARCHAR(20) NULL,
    [InvoiceNo] VARCHAR(50) NULL,
    [RowNo] SMALLINT NULL,
    [Narration] TEXT NULL,
    [TransType] VARCHAR(255) NULL
);

CREATE TABLE [TempSelfTradingPendingDheries] (
    [Item1] VARCHAR(255) NULL,
    [Item2] VARCHAR(255) NULL,
    [Item3] VARCHAR(255) NULL,
    [Item4] VARCHAR(255) NULL,
    [Item5] VARCHAR(255) NULL,
    [Item6] VARCHAR(255) NULL,
    [Item7] VARCHAR(255) NULL,
    [Item8] VARCHAR(255) NULL,
    [Item9] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL
);

CREATE TABLE [TempShowAPIBalTable] (
    [ItemSrNo] INTEGER NULL,
    [FieldNm] VARCHAR(255) NULL,
    [FieldValue] VARCHAR(255) NULL
);

CREATE TABLE [TempShowDateStockForItems] (
    [ItemName] VARCHAR(255) NULL,
    [ItemType] VARCHAR(255) NULL,
    [UnitName] VARCHAR(255) NULL,
    [Packing] DOUBLE NULL,
    [SaleRate] DOUBLE NULL,
    [BagsSelfPurchase] DOUBLE NULL,
    [CurrentBalance] DOUBLE NULL,
    [VAT] DOUBLE NULL,
    [VATLowerRate] DOUBLE NULL,
    [CST] DOUBLE NULL,
    [CSTWithoutCForm] DOUBLE NULL,
    [Code1st] INTEGER NULL,
    [RowNo] INTEGER NULL
);

CREATE TABLE [TempStockReport] (
    [CompanyName] VARCHAR(255) NULL,
    [VehicleModal] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [ItemCode] VARCHAR(255) NULL,
    [PurchaseRate] DOUBLE NULL,
    [SaleRate] DOUBLE NULL,
    [ClosingStock] VARCHAR(255) NULL,
    [Location] VARCHAR(255) NULL,
    [Col1] VARCHAR(255) NULL,
    [Col2] VARCHAR(255) NULL,
    [Col3] VARCHAR(255) NULL,
    [Col4] VARCHAR(255) NULL,
    [Col5] VARCHAR(255) NULL
);

CREATE TABLE [TempTable] (
    [RowNo] INTEGER NULL,
    [Item1] VARCHAR(255) NULL,
    [Item1Qty] DOUBLE NULL,
    [Item2] VARCHAR(255) NULL,
    [Item3] VARCHAR(255) NULL,
    [Item4] VARCHAR(255) NULL,
    [Item5] VARCHAR(255) NULL,
    [Item6] VARCHAR(255) NULL,
    [Item7] VARCHAR(255) NULL,
    [Item8] VARCHAR(255) NULL,
    [Item9] VARCHAR(255) NULL,
    [Item10] VARCHAR(255) NULL
);

CREATE TABLE [TempTable1] (
    [RowNo] INTEGER NOT NULL,
    [Item1] VARCHAR(255) NULL,
    [Item2] VARCHAR(255) NULL,
    [Item3] VARCHAR(255) NULL,
    [Item4] VARCHAR(255) NULL,
    [Item5] VARCHAR(255) NULL,
    [Item6] VARCHAR(255) NULL,
    [Item7] VARCHAR(255) NULL,
    [Item8] VARCHAR(255) NULL,
    [Item9] VARCHAR(255) NULL,
    [Item10] VARCHAR(255) NULL
);

CREATE TABLE [TempTDSReport] (
    [PartyCode] SMALLINT NULL,
    [VoucherDate] DATETIME NULL,
    [TransType] VARCHAR(6) NULL,
    [Amount] DOUBLE NULL,
    [TDS] DOUBLE NULL,
    [VoucherNumber] SMALLINT NULL
);

CREATE TABLE [TempTrialBalance] (
    [FigureName] VARCHAR(255) NULL,
    [DrAmount] DOUBLE NULL,
    [CrAmount] DOUBLE NULL,
    [SizeFont] INTEGER NULL,
    [BoldFont] INTEGER NULL,
    [UnderlineFont] INTEGER NULL,
    [ItalicFont] INTEGER NULL,
    [RowNo] INTEGER NULL,
    [DrAmountString] VARCHAR(255) NULL,
    [CrAmountString] VARCHAR(255) NULL
);

CREATE TABLE [TempVATC4] (
    [RowNo] INTEGER NULL,
    [SalePartyName] VARCHAR(255) NULL,
    [SalePartyTIN] VARCHAR(255) NULL,
    [PurcPartyName] VARCHAR(255) NULL,
    [PurcPartyTIN] VARCHAR(255) NULL,
    [SrNo] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [InvoiceNo] VARCHAR(255) NULL,
    [InvoiceDate] VARCHAR(255) NULL,
    [TaxableAmount] VARCHAR(255) NULL,
    [Tax] VARCHAR(255) NULL,
    [GroupName] VARCHAR(255) NULL,
    [District] VARCHAR(255) NULL,
    [GrossAmt] VARCHAR(255) NULL
);

CREATE TABLE [TempVATReport] (
    [Item1] VARCHAR(255) NULL,
    [Item1Alignment] INTEGER NULL,
    [Item1FontSize] INTEGER NULL,
    [Item1Bold] INTEGER NULL,
    [Item1Underline] INTEGER NULL,
    [Item2] VARCHAR(255) NULL,
    [Item2Alignment] INTEGER NULL,
    [Item2FontSize] INTEGER NULL,
    [Item2Bold] INTEGER NULL,
    [Item2Underline] INTEGER NULL,
    [Item3] VARCHAR(255) NULL,
    [Item3Alignment] INTEGER NULL,
    [Item3FontSize] INTEGER NULL,
    [Item3Bold] INTEGER NULL,
    [Item3Underline] INTEGER NULL,
    [RowNo] INTEGER NULL
);

CREATE TABLE [TimberVouchers] (
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [RowNo] INTEGER NULL,
    [EnteredItemName] VARCHAR(255) NULL,
    [F1] VARCHAR(255) NULL,
    [F2] VARCHAR(255) NULL,
    [F3] VARCHAR(255) NULL,
    [F4] VARCHAR(255) NULL,
    [F5] VARCHAR(255) NULL,
    [F6] VARCHAR(255) NULL,
    [F7] VARCHAR(255) NULL,
    [F8] VARCHAR(255) NULL,
    [F9] VARCHAR(255) NULL,
    [F10] VARCHAR(255) NULL,
    [F11] VARCHAR(255) NULL,
    [F12] VARCHAR(255) NULL,
    [F13] VARCHAR(255) NULL,
    [F14] VARCHAR(255) NULL,
    [F15] VARCHAR(255) NULL,
    [F16] VARCHAR(255) NULL,
    [F17] VARCHAR(255) NULL,
    [F18] VARCHAR(255) NULL,
    [F19] VARCHAR(255) NULL,
    [F20] VARCHAR(255) NULL,
    [F21] VARCHAR(255) NULL,
    [F22] VARCHAR(255) NULL,
    [ItemName] VARCHAR(255) NULL,
    [SrNo] INTEGER NULL
);

CREATE TABLE [Transactions] (
    [RowNo] SMALLINT NULL,
    [VoucherNumber] INTEGER NULL,
    [VoucherDate] DATETIME NULL,
    [TransType] VARCHAR(5) NOT NULL,
    [AccountCode] SMALLINT NOT NULL,
    [DrCr] VARCHAR(3) NOT NULL,
    [Amount] DOUBLE NOT NULL,
    [InvoiceNo] VARCHAR(50) NULL,
    [PartyCode] SMALLINT NULL,
    [JFormSaleStatus] VARCHAR(20) NULL,
    [IFormTaxStatus] VARCHAR(4) NULL,
    [IFormMarketFeeStatus] VARCHAR(8) NULL,
    [PurchaseType] VARCHAR(30) NULL,
    [CashCreditStatus] VARCHAR(7) NULL,
    [Narration] VARCHAR(200) NOT NULL,
    [EntryType] VARCHAR(25) NULL,
    [DueDays] SMALLINT NULL,
    [ItemCode] SMALLINT NULL,
    [SalePurcAgainst] VARCHAR(255) NULL,
    [CashPartyName] VARCHAR(255) NULL,
    [CashPartyTIN] VARCHAR(255) NULL,
    [CashPartyLedgerCode] INTEGER NULL,
    [ReturnDateForTax] DATETIME NULL,
    [ReturnDateENTERED] INTEGER NULL,
    [FormVAT47No] VARCHAR(255) NULL,
    [HideAmount] DOUBLE NULL,
    [FormQuery] INTEGER NULL,
    [ZimidarName] VARCHAR(255) NULL,
    [SaudaDate] VARCHAR(255) NULL,
    [BankDate] DATETIME NULL,
    [VoucherReconcile] INTEGER NULL,
    [DebitCreditNote] INTEGER NULL,
    [BankDateStatus] DATETIME NULL,
    [VoucherReconcileStatus] INTEGER NULL,
    [TransCD] DOUBLE NULL,
    [ExpRate] DOUBLE NULL,
    [ExpUnit] VARCHAR(255) NULL,
    [ImagePath] VARCHAR(255) NULL,
    [BrokerName] VARCHAR(255) NULL,
    [EstimatedAmount] DOUBLE NULL,
    [RTGSBank] VARCHAR(255) NULL,
    [BalAmount] DOUBLE NULL,
    [MktCommttSrNo] INTEGER NULL,
    [ReturnDate] DATETIME NULL,
    [OtherFormApply] VARCHAR(255) NULL,
    [URDPurc] INTEGER NULL,
    [E1PartyCode] INTEGER NULL,
    [E1Details1] VARCHAR(255) NULL,
    [E1Details2] VARCHAR(255) NULL,
    [E1Details3] VARCHAR(255) NULL,
    [CompositionVch] INTEGER NULL,
    [TaxInvoiceNo] VARCHAR(255) NULL,
    [PlaceOfSupply] VARCHAR(255) NULL,
    [ECommGSTIN] VARCHAR(255) NULL,
    [TransReturn] INTEGER NULL,
    [GroupTick] INTEGER NULL,
    [SaudaDateIntt] DATETIME NULL,
    [ActualInv] INTEGER NULL,
    [ITCNotClaim] INTEGER NULL,
    [ReverseChargePayable] INTEGER NULL,
    [GSTOnGoodsAmount] INTEGER NULL,
    [Spare1] VARCHAR(255) NULL,
    [TCSRate] REAL NULL,
    [TCSTaxable] DOUBLE NULL,
    [DrCrNoteMode] VARCHAR(255) NULL,
    [ChallanVchNo] INTEGER NULL,
    [ChallanVchDATE] DATETIME NULL,
    [TempInv] VARCHAR(255) NULL,
    [TDSRate194Q] REAL NULL,
    [Taxable194Q] DOUBLE NULL,
    [TDS194QChallanVchNo] INTEGER NULL,
    [TDS194QChallanVchDate] DATETIME NULL,
    [TDS194QChallanTransType] VARCHAR(255) NULL,
    [TaxInputDate] DATETIME NULL,
    [PymtDone] INTEGER NULL,
    [tmpBookNo] DOUBLE NULL,
    [tmpSlipNo] DOUBLE NULL,
    [LtNo] VARCHAR(255) NULL,
    [RunningBNo] DOUBLE NULL
);

CREATE TABLE [VerifiedVouchers] (
    [AccountCode] INTEGER NULL,
    [VoucherDate] DATETIME NULL,
    [VoucherNumber] INTEGER NULL,
    [TransType] VARCHAR(255) NULL,
    [YearFrom] DATETIME NULL,
    [YearTo] DATETIME NULL,
    [TempRemove] INTEGER NULL,
    [Narration] VARCHAR(255) NULL,
    [DrCr] VARCHAR(255) NULL,
    [Amount] DOUBLE NULL
);

CREATE TABLE [VoucherSettings] (
    [VoucherType] VARCHAR(6) NULL,
    [FirstTotalRoundOff] BOOLEAN NOT NULL,
    [DamiRoundOff] BOOLEAN NOT NULL,
    [LabourRoundOff] BOOLEAN NOT NULL,
    [MarketFeeRoundOff] BOOLEAN NOT NULL,
    [HRDFRoundOff] BOOLEAN NOT NULL,
    [BonusRoundOff] BOOLEAN NOT NULL,
    [ReliefRoundOff] BOOLEAN NOT NULL,
    [VATRoundOff] BOOLEAN NOT NULL,
    [Roundoff] BOOLEAN NOT NULL,
    [TaxIncludeInFirstAmount] BOOLEAN NOT NULL,
    [BagsInclude] BOOLEAN NOT NULL,
    [PackingInclude] BOOLEAN NOT NULL,
    [WeightInclude] BOOLEAN NOT NULL,
    [Welfare] DOUBLE NULL,
    [Gaushala] DOUBLE NULL,
    [Brokerage] DOUBLE NULL,
    [Dharmada] DOUBLE NULL,
    [WelfareLedger] DOUBLE NULL,
    [GaushalaLedger] DOUBLE NULL,
    [DharmadaLedger] DOUBLE NULL,
    [BrokerageLedger] DOUBLE NULL,
    [WelfareExpLedger] DOUBLE NULL,
    [GaushalaExpLedger] DOUBLE NULL,
    [DharmadaExpLedger] DOUBLE NULL,
    [BrokerageExpLedger] DOUBLE NULL,
    [LabourExpLedger] DOUBLE NULL,
    [StandardRoundOff] INTEGER NULL,
    [DamiTDSRoundOff] INTEGER NULL,
    [DamiTDSAllFieldsRoundOff] INTEGER NULL,
    [CalculateVATonFreight] INTEGER NULL,
    [CalculateCommissiononFreight] INTEGER NULL,
    [AutoCalculationEnabled] INTEGER NULL,
    [ShowDateStockForItems] INTEGER NULL,
    [OtherPurcAsMandiType] INTEGER NULL,
    [PromptForTransferRaj] INTEGER NULL,
    [PrintInvoiceAsEstimate] INTEGER NULL,
    [LabourOnAmount] INTEGER NULL,
    [BillPrintFormat] VARCHAR(255) NULL,
    [BillLogoPath] VARCHAR(255) NULL,
    [CSTInPurc] INTEGER NULL,
    [AllowZeroAmount] INTEGER NULL,
    [RateUnit1st] INTEGER NULL,
    [PromptForItem] INTEGER NULL,
    [PromptItemNarration] INTEGER NULL,
    [ItemNarrationPrefix] VARCHAR(255) NULL,
    [ShowVehNoInfo] INTEGER NULL,
    [PrintString] VARCHAR(255) NULL,
    [ShowSaudaDate] INTEGER NULL,
    [PrintCopies] INTEGER NULL,
    [WeightNotShowInBillPrinting] INTEGER NULL,
    [PrintTaxRate] INTEGER NULL,
    [ShowBillAmountInLagatBillNarration] INTEGER NULL,
    [MarketFeeNotChangeInPurchaseAlteration] INTEGER NULL,
    [PromptForSMS] INTEGER NULL,
    [PromptDiscountPercentage] INTEGER NULL,
    [PromptForPrinting] INTEGER NULL,
    [PromptForBrokerName] INTEGER NULL,
    [MinusStockNotPick] INTEGER NULL,
    [BillStamp] VARCHAR(255) NULL,
    [PromptForCash] INTEGER NULL,
    [MktCommttName] VARCHAR(255) NULL,
    [District] VARCHAR(255) NULL,
    [SecretartyName] VARCHAR(255) NULL,
    [SecretaryContactNo] VARCHAR(255) NULL,
    [MktCommttContactNo] VARCHAR(255) NULL,
    [MktCommttPrintCopies] INTEGER NULL,
    [ShowItemNarrationInLedger] INTEGER NULL,
    [TimeAutoPick] INTEGER NULL,
    [BillNoSaleIFrmJoint] INTEGER NULL,
    [BillPrintonLetterPad] INTEGER NULL,
    [TaxableAmountAutomaticCotton] INTEGER NULL,
    [BillInterestLine] VARCHAR(255) NULL,
    [BillIshtdevLine] VARCHAR(255) NULL,
    [SimpleStockShow] INTEGER NULL,
    [ReceiptPlanWithoutKalamReceipt] INTEGER NULL,
    [PrintPartyBalance] INTEGER NULL,
    [PrintLedgerNameAsExpHead] INTEGER NULL,
    [MFeeHRDFCalcOnBags] INTEGER NULL,
    [CalcSingleItemOpClInTrading] INTEGER NULL,
    [UseAgencyTypeBill] INTEGER NULL,
    [PrintNarrWithItemName] INTEGER NULL,
    [smsAPIcode] VARCHAR(255) NULL,
    [smsSignature] VARCHAR(255) NULL,
    [MakeSameRateDheriesJoint] INTEGER NULL,
    [BrokerNameNotPrint] INTEGER NULL,
    [AdditionalDetailsShowInLedger] INTEGER NULL,
    [PromptForFreightTDS] INTEGER NULL,
    [PromptForAdditionalExp] INTEGER NULL,
    [PrintCompanyWithItemName] INTEGER NULL,
    [RateAutoChange] INTEGER NULL,
    [PurcTaxOnGoodsAmount] INTEGER NULL,
    [PromptMonthOrTotalYear] INTEGER NULL,
    [PromptForBillSeries] INTEGER NULL,
    [ShowSAPCode] INTEGER NULL,
    [AutoSetSGSTCGSTOnePaisa] INTEGER NULL,
    [StockTransferHeading] VARCHAR(255) NULL,
    [OtherExpJointPrint] INTEGER NULL,
    [ItemNameFullPrint] INTEGER NULL,
    [UtraiCalcOnBags] INTEGER NULL,
    [AmountPrintIncludingTax] INTEGER NULL,
    [Unit1stPrintHeading] VARCHAR(255) NULL,
    [Unit2ndPrintHeading] VARCHAR(255) NULL,
    [SenderID] VARCHAR(255) NULL,
    [UserNm] VARCHAR(255) NULL,
    [PassWd] VARCHAR(255) NULL,
    [AuthKey] VARCHAR(255) NULL,
    [PromptForBillDate] INTEGER NULL,
    [BillQRCodePath] VARCHAR(255) NULL,
    [BillSignaturePath] VARCHAR(255) NULL,
    [EmailID] VARCHAR(255) NULL,
    [EmailPasswd] VARCHAR(255) NULL,
    [NarrPrintasPartyName] INTEGER NULL,
    [AppOtherMobNo] TEXT NULL,
    [NagShowInPrint] INTEGER NULL,
    [LessCommissionLedger] INTEGER NULL,
    [ApplyTCS] INTEGER NULL,
    [TCSPartyTurnover] DOUBLE NULL,
    [TCSRateNormal] DOUBLE NULL,
    [TCSRateWithoutPAN] DOUBLE NULL,
    [AutoCheckTCS] INTEGER NULL,
    [TCSRoundOff] INTEGER NULL,
    [PurchaseTCSLedger] INTEGER NULL,
    [SaleTCSLedger] INTEGER NULL,
    [CountryCode] INTEGER NULL,
    [EInvoiceFilePath] VARCHAR(255) NULL,
    [GSTNUserName] VARCHAR(255) NULL,
    [GSTNPasswd] VARCHAR(255) NULL,
    [EWBUserName] VARCHAR(255) NULL,
    [EWBPasswd] VARCHAR(255) NULL,
    [YourID] VARCHAR(255) NULL,
    [AutoEInvoice] INTEGER NULL,
    [EntityID] VARCHAR(255) NULL,
    [ApplyTDS194Q] INTEGER NULL,
    [TDS194QLedger] INTEGER NULL,
    [TDS194QRoundOff] INTEGER NULL,
    [ApplyTCSInSale] INTEGER NULL,
    [ApplyTCSInPurc] INTEGER NULL,
    [ExpAddTaxFreeSale] INTEGER NULL,
    [AppPwd] VARCHAR(255) NULL,
    [PromptForPrintWeightNote] INTEGER NULL,
    [GSTPortalUserName] VARCHAR(255) NULL,
    [GSTPortalOTP] VARCHAR(255) NULL,
    [CommNotOnBardana] INTEGER NULL,
    [ZeroStockNotShow] INTEGER NULL,
    [WeightDigits] INTEGER NULL,
    [MakeJSONForEInvoice] INTEGER NULL,
    [CottonBargainRegister] INTEGER NULL,
    [PrintDharaRate] INTEGER NULL,
    [AutoOnlyEWayBill] INTEGER NULL,
    [PromptBagsForIFormInJForm] INTEGER NULL,
    [AlwaysUseCurrentDateInSale] INTEGER NULL,
    [SaleInvoiceNoNotRequire] INTEGER NULL
);

