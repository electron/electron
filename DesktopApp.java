import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class CalculatorApp {

    private static JTextField textField;

    public static void main(String[] args) {
        JFrame frame = new JFrame("Calculator");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLayout(new BorderLayout());

        textField = new JTextField();
        textField.setFont(new Font("Arial", Font.PLAIN, 24));
        textField.setHorizontalAlignment(SwingConstants.RIGHT);
        frame.add(textField, BorderLayout.NORTH);

        JPanel buttonPanel = new JPanel();
        buttonPanel.setLayout(new GridLayout(4, 4));
        frame.add(buttonPanel, BorderLayout.CENTER);

        String[] buttonLabels = {
                "7", "8", "9", "/",
                "4", "5", "6", "*",
                "1", "2", "3", "-",
                "0", ".", "=", "+"
        };

        for (String label : buttonLabels) {
            JButton button = new JButton(label);
            button.setFont(new Font("Arial", Font.PLAIN, 24));
            button.addActionListener(new ButtonClickListener());
            buttonPanel.add(button);
        }

        frame.setSize(400, 500);
        frame.setVisible(true);
    }

    private static class ButtonClickListener implements ActionListener {
        public void actionPerformed(ActionEvent event) {
            JButton source = (JButton) event.getSource();
            String buttonText = source.getText();

            if (buttonText.equals("=")) {
                String expression = textField.getText();
                try {
                    double result = evaluateExpression(expression);
                    textField.setText(Double.toString(result));
                } catch (Exception e) {
                    textField.setText("Error");
                }
            } else {
                String currentText = textField.getText();
                textField.setText(currentText + buttonText);
            }
        }

        private double evaluateExpression(String expression) {
            // Basic evaluation logic (not handling parentheses, complex expressions, etc.)
            String[] tokens = expression.split("[*/+-]");
            double operand1 = Double.parseDouble(tokens[0]);
            double operand2 = Double.parseDouble(tokens[1]);

            if (expression.contains("+")) {
                return operand1 + operand2;
            } else if (expression.contains("-")) {
                return operand1 - operand2;
            } else if (expression.contains("*")) {
                return operand1 * operand2;
            } else if (expression.contains("/")) {
                return operand1 / operand2;
            } else {
                throw new IllegalArgumentException("Invalid expression");
            }
        }
    }
}



