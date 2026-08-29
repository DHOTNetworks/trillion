import sys
import os
from PySide6.QtGui import QGuiApplication
from PySide6.QtQml import QQmlApplicationEngine
from PySide6.QtQuickControls2 import QQuickStyle
from PySide6.QtCore import QUrl

from backend.database import init_db
from backend.models import (
    PaddyArrivalsModel, 
    MillingModel, 
    SalesModel, 
    PurchaseModel,
    VouchersModel, 
    PartiesModel, 
    AccountGroupsModel,
    StockItemsModel,
    DashboardController
)

def main():
    # Force Qt Quick Controls to use 'Basic' style to allow custom dark theme rendering
    QQuickStyle.setStyle("Basic")

    # Initialize SQLite Database & Tables
    init_db()

    app = QGuiApplication(sys.argv)
    app.setOrganizationName("Mahadev Rice Mill")
    app.setApplicationName("Mahadev Rice Mill ERP & Accounting")

    engine = QQmlApplicationEngine()

    # Instantiate Backend Models
    dashboard_ctrl = DashboardController()
    paddy_model = PaddyArrivalsModel()
    milling_model = MillingModel()
    sales_model = SalesModel()
    purchase_model = PurchaseModel()
    vouchers_model = VouchersModel()
    parties_model = PartiesModel()
    groups_model = AccountGroupsModel()
    stock_items_model = StockItemsModel()

    # Expose models to QML context
    root_context = engine.rootContext()
    root_context.setContextProperty("dashboardCtrl", dashboard_ctrl)
    root_context.setContextProperty("paddyModel", paddy_model)
    root_context.setContextProperty("millingModel", milling_model)
    root_context.setContextProperty("salesModel", sales_model)
    root_context.setContextProperty("purchaseModel", purchase_model)
    root_context.setContextProperty("vouchersModel", vouchers_model)
    root_context.setContextProperty("partiesModel", parties_model)
    root_context.setContextProperty("groupsModel", groups_model)
    root_context.setContextProperty("stockItemsModel", stock_items_model)

    # Load main QML interface
    base_path = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    qml_file = os.path.join(base_path, "qml", "main.qml")
    engine.load(QUrl.fromLocalFile(qml_file))

    if not engine.rootObjects():
        print("Failed to load QML interface.")
        sys.exit(-1)

    print("Mahadev Rice Mill Accounting ERP launched successfully!")
    sys.exit(app.exec())

if __name__ == "__main__":
    main()
