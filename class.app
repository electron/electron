import javafx.application.Application;
import javafx.collections.FXCollections;
import javafx.collections.ObservableList;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.layout.VBox;
import javafx.stage.Stage;

public class AddressBookApp extends Application {
    private final ObservableList<Contact> contacts = FXCollections.observableArrayList();

    private final TableView<Contact> table = new TableView<>();

    private final TextField nameField = new TextField();
    private final TextField emailField = new TextField();

    public static void main(String[] args) {
        launch(args);
    }

    @Override
    public void start(Stage primaryStage) {
        primaryStage.setTitle("Address Book");

        table.setColumnResizePolicy(TableView.CONSTRAINED_RESIZE_POLICY);

        TableColumn<Contact, String> nameColumn = new TableColumn<>("Name");
        nameColumn.setCellValueFactory(cellData -> cellData.getValue().nameProperty());

        TableColumn<Contact, String> emailColumn = new TableColumn<>("Email");
        emailColumn.setCellValueFactory(cellData -> cellData.getValue().emailProperty());

        table.getColumns().addAll(nameColumn, emailColumn);

        table.setItems(contacts);

        Button addButton = new Button("Add");
        addButton.setOnAction(e -> addContact());

        Button editButton = new Button("Edit");
        editButton.setOnAction(e -> editContact());

        Button deleteButton = new Button("Delete");
        deleteButton.setOnAction(e -> deleteContact());

        VBox vBox = new VBox();
        vBox.getChildren().addAll(table, nameField, emailField, addButton, editButton, deleteButton);

        Scene scene = new Scene(vBox, 400, 400);
        primaryStage.setScene(scene);
        primaryStage.show();
    }

    private void addContact() {
        String name = nameField.getText();
        String email = emailField.getText();

        if (!name.isEmpty() && !email.isEmpty()) {
            contacts.add(new Contact(name, email));
            nameField.clear();
            emailField.clear();
        }
    }

    private void editContact() {
        Contact selectedContact = table.getSelectionModel().getSelectedItem();
        if (selectedContact != null) {
            String name = nameField.getText();
            String email = emailField.getText();

            if (!name.isEmpty() && !email.isEmpty()) {
                selectedContact.setName(name);
                selectedContact.setEmail(email);
                nameField.clear();
                emailField.clear();
            }
        }
    }

    private void deleteContact() {
        Contact selectedContact = table.getSelectionModel().getSelectedItem();
        if (selectedContact != null) {
            contacts.remove(selectedContact);
            nameField.clear();
            emailField.clear();
        }
    }
}

class Contact {
    private final String name;
    private final String email;

    public Contact(String name, String email) {
        this.name = name;
        this.email = email;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        // You can add validation logic here.
        this.name = name;
    }

    public String getEmail() {
        return email;
    }

    public void setEmail(String email) {
        // You can add validation logic here.
        this.email = email;
    }

    // Properties for JavaFX
    public StringProperty nameProperty() {
        return new SimpleStringProperty(name);
    }

    public StringProperty emailProperty() {
        return new SimpleStringProperty(email);
    }
}
